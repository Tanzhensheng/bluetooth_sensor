#include "ble_dbus_app.h"

#include <stdio.h>
#include <string.h>

#include "app_log.h"

#ifdef __linux__

#include <gio/gio.h>

#define APP_CONN(app) ((GDBusConnection *)(app)->connection)
#define APP_ROOT_NODE(app) ((GDBusNodeInfo *)(app)->root_node_info)
#define APP_SERVICE_NODE(app) ((GDBusNodeInfo *)(app)->service_node_info)
#define APP_RX_NODE(app) ((GDBusNodeInfo *)(app)->rx_node_info)
#define APP_TX_NODE(app) ((GDBusNodeInfo *)(app)->tx_node_info)
#define APP_AD_NODE(app) ((GDBusNodeInfo *)(app)->ad_node_info)

static const char *k_root_xml =
    "<node>"
    "  <interface name='org.freedesktop.DBus.ObjectManager'>"
    "    <method name='GetManagedObjects'>"
    "      <arg type='a{oa{sa{sv}}}' name='objects' direction='out'/>"
    "    </method>"
    "  </interface>"
    "</node>";

static const char *k_service_xml =
    "<node>"
    "  <interface name='org.bluez.GattService1'>"
    "    <property name='UUID' type='s' access='read'/>"
    "    <property name='Primary' type='b' access='read'/>"
    "  </interface>"
    "</node>";

static const char *k_rx_char_xml =
    "<node>"
    "  <interface name='org.bluez.GattCharacteristic1'>"
    "    <method name='WriteValue'>"
    "      <arg type='ay' name='value' direction='in'/>"
    "      <arg type='a{sv}' name='options' direction='in'/>"
    "    </method>"
    "    <property name='UUID' type='s' access='read'/>"
    "    <property name='Service' type='o' access='read'/>"
    "    <property name='Flags' type='as' access='read'/>"
    "    <property name='Value' type='ay' access='read'/>"
    "  </interface>"
    "</node>";

static const char *k_tx_char_xml =
    "<node>"
    "  <interface name='org.bluez.GattCharacteristic1'>"
    "    <method name='StartNotify'/>"
    "    <method name='StopNotify'/>"
    "    <property name='UUID' type='s' access='read'/>"
    "    <property name='Service' type='o' access='read'/>"
    "    <property name='Flags' type='as' access='read'/>"
    "    <property name='Notifying' type='b' access='read'/>"
    "    <property name='Value' type='ay' access='read'/>"
    "  </interface>"
    "</node>";

static const char *k_ad_xml =
    "<node>"
    "  <interface name='org.bluez.LEAdvertisement1'>"
    "    <method name='Release'/>"
    "    <property name='Type' type='s' access='read'/>"
    "    <property name='ServiceUUIDs' type='as' access='read'/>"
    "    <property name='LocalName' type='s' access='read'/>"
    "  </interface>"
    "</node>";

static GVariant *new_string_array(const char *const *items) {
    GVariantBuilder builder;
    unsigned i;

    g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));
    for (i = 0; items[i] != NULL; ++i) {
        g_variant_builder_add(&builder, "s", items[i]);
    }
    return g_variant_builder_end(&builder);
}

static GVariant *new_byte_array(const unsigned char *buf, unsigned len) {
    return g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE, buf, len, sizeof(guchar));
}

static int find_adapter_path(ble_dbus_app_t *app) {
    GError *error = NULL;
    GVariant *reply = NULL;
    GVariant *managed = NULL;
    GVariantIter iter;
    const char *object_path = NULL;
    GVariant *ifaces = NULL;

    reply = g_dbus_connection_call_sync(
        APP_CONN(app),
        "org.bluez",
        "/",
        "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects",
        NULL,
        G_VARIANT_TYPE("(a{oa{sa{sv}}})"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );
    if (reply == NULL) {
        app_log_error("GetManagedObjects failed: %s", error != NULL ? error->message : "unknown");
        g_clear_error(&error);
        return -1;
    }

    managed = g_variant_get_child_value(reply, 0);
    g_variant_iter_init(&iter, managed);
    while (g_variant_iter_loop(&iter, "{&o@a{sa{sv}}}", &object_path, &ifaces)) {
        GVariantIter if_iter;
        const char *if_name = NULL;
        GVariant *props = NULL;
        int has_gatt = 0;
        int has_adv = 0;

        g_variant_iter_init(&if_iter, ifaces);
        while (g_variant_iter_loop(&if_iter, "{&s@a{sv}}", &if_name, &props)) {
            if (strcmp(if_name, "org.bluez.GattManager1") == 0) {
                has_gatt = 1;
            }
            if (strcmp(if_name, "org.bluez.LEAdvertisingManager1") == 0) {
                has_adv = 1;
            }
        }

        if (has_gatt != 0 && has_adv != 0) {
            snprintf(app->adapter_path, sizeof(app->adapter_path), "%s", object_path);
            break;
        }
    }

    g_variant_unref(managed);
    g_variant_unref(reply);
    return (app->adapter_path[0] != '\0') ? 0 : -1;
}

static void add_service_properties(const ble_dbus_app_t *app, GVariantBuilder *props) {
    g_variant_builder_add(props, "{sv}", "UUID", g_variant_new_string(app->service_uuid));
    g_variant_builder_add(props, "{sv}", "Primary", g_variant_new_boolean(TRUE));
}

static void add_rx_properties(const ble_dbus_app_t *app, GVariantBuilder *props) {
    static const char *const flags[] = {"write", NULL};
    unsigned char empty = 0U;

    g_variant_builder_add(props, "{sv}", "UUID", g_variant_new_string(app->rx_uuid));
    g_variant_builder_add(props, "{sv}", "Service", g_variant_new_object_path(app->service_path));
    g_variant_builder_add(props, "{sv}", "Flags", new_string_array(flags));
    g_variant_builder_add(props, "{sv}", "Value", new_byte_array(&empty, 0U));
}

static void add_tx_properties(const ble_dbus_app_t *app, GVariantBuilder *props) {
    static const char *const flags[] = {"notify", NULL};

    g_variant_builder_add(props, "{sv}", "UUID", g_variant_new_string(app->tx_uuid));
    g_variant_builder_add(props, "{sv}", "Service", g_variant_new_object_path(app->service_path));
    g_variant_builder_add(props, "{sv}", "Flags", new_string_array(flags));
    g_variant_builder_add(props, "{sv}", "Notifying", g_variant_new_boolean(app->notify_enabled != 0));
    g_variant_builder_add(props, "{sv}", "Value", new_byte_array(app->tx_value, app->tx_len));
}

static void add_char_entry(GVariantBuilder *objects, const char *path, const char *iface_name, void (*fill_props)(const ble_dbus_app_t *, GVariantBuilder *), const ble_dbus_app_t *app) {
    GVariantBuilder ifaces;
    GVariantBuilder props;

    g_variant_builder_init(&ifaces, G_VARIANT_TYPE("a{sa{sv}}"));
    g_variant_builder_init(&props, G_VARIANT_TYPE("a{sv}"));
    fill_props(app, &props);
    g_variant_builder_add(&ifaces, "{s@a{sv}}", iface_name, g_variant_builder_end(&props));
    g_variant_builder_add(objects, "{o@a{sa{sv}}}", path, g_variant_builder_end(&ifaces));
}

static void root_method_call(
    GDBusConnection *connection,
    const char *sender,
    const char *object_path,
    const char *interface_name,
    const char *method_name,
    GVariant *parameters,
    GDBusMethodInvocation *invocation,
    gpointer user_data
) {
    ble_dbus_app_t *app = (ble_dbus_app_t *)user_data;
    GVariantBuilder objects;

    (void)connection;
    (void)sender;
    (void)object_path;
    (void)interface_name;
    (void)parameters;

    if (strcmp(method_name, "GetManagedObjects") != 0) {
        g_dbus_method_invocation_return_dbus_error(invocation, "org.freedesktop.DBus.Error.NotSupported", "Unsupported method");
        return;
    }

    g_variant_builder_init(&objects, G_VARIANT_TYPE("a{oa{sa{sv}}}"));
    add_char_entry(&objects, app->service_path, "org.bluez.GattService1", add_service_properties, app);
    add_char_entry(&objects, app->rx_path, "org.bluez.GattCharacteristic1", add_rx_properties, app);
    add_char_entry(&objects, app->tx_path, "org.bluez.GattCharacteristic1", add_tx_properties, app);
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(a{oa{sa{sv}}})", &objects));
}

static void rx_method_call(
    GDBusConnection *connection,
    const char *sender,
    const char *object_path,
    const char *interface_name,
    const char *method_name,
    GVariant *parameters,
    GDBusMethodInvocation *invocation,
    gpointer user_data
) {
    ble_dbus_app_t *app = (ble_dbus_app_t *)user_data;
    GVariant *value = NULL;
    gsize len = 0;
    const unsigned char *bytes = NULL;

    (void)connection;
    (void)sender;
    (void)object_path;
    (void)interface_name;

    if (strcmp(method_name, "WriteValue") != 0) {
        g_dbus_method_invocation_return_dbus_error(invocation, "org.freedesktop.DBus.Error.NotSupported", "Unsupported method");
        return;
    }

    value = g_variant_get_child_value(parameters, 0);
    bytes = (const unsigned char *)g_variant_get_fixed_array(value, &len, sizeof(guchar));
    if (bytes != NULL && len > 0U) {
        app_log_hex("RX", bytes, (unsigned)len);
        if (app->rx_cb != NULL) {
            app->rx_cb(bytes, (unsigned)len, app->user);
        }
    }
    g_variant_unref(value);
    g_dbus_method_invocation_return_value(invocation, NULL);
}

static void tx_method_call(
    GDBusConnection *connection,
    const char *sender,
    const char *object_path,
    const char *interface_name,
    const char *method_name,
    GVariant *parameters,
    GDBusMethodInvocation *invocation,
    gpointer user_data
) {
    ble_dbus_app_t *app = (ble_dbus_app_t *)user_data;

    (void)connection;
    (void)sender;
    (void)object_path;
    (void)interface_name;
    (void)parameters;

    if (strcmp(method_name, "StartNotify") == 0) {
        app->notify_enabled = 1;
        if (app->event_cb != NULL) {
            app->event_cb(TRANSPORT_EVENT_NOTIFY_ENABLED, app->user);
        }
        g_dbus_method_invocation_return_value(invocation, NULL);
        return;
    }

    if (strcmp(method_name, "StopNotify") == 0) {
        app->notify_enabled = 0;
        if (app->event_cb != NULL) {
            app->event_cb(TRANSPORT_EVENT_NOTIFY_DISABLED, app->user);
        }
        g_dbus_method_invocation_return_value(invocation, NULL);
        return;
    }

    g_dbus_method_invocation_return_dbus_error(invocation, "org.freedesktop.DBus.Error.NotSupported", "Unsupported method");
}

static void ad_method_call(
    GDBusConnection *connection,
    const char *sender,
    const char *object_path,
    const char *interface_name,
    const char *method_name,
    GVariant *parameters,
    GDBusMethodInvocation *invocation,
    gpointer user_data
) {
    (void)connection;
    (void)sender;
    (void)object_path;
    (void)interface_name;
    (void)parameters;
    (void)user_data;

    if (strcmp(method_name, "Release") == 0) {
        g_dbus_method_invocation_return_value(invocation, NULL);
        return;
    }

    g_dbus_method_invocation_return_dbus_error(invocation, "org.freedesktop.DBus.Error.NotSupported", "Unsupported method");
}

static GVariant *service_get_property(
    GDBusConnection *connection,
    const char *sender,
    const char *object_path,
    const char *interface_name,
    const char *property_name,
    GError **error,
    gpointer user_data
) {
    ble_dbus_app_t *app = (ble_dbus_app_t *)user_data;

    (void)connection;
    (void)sender;
    (void)object_path;
    (void)interface_name;
    (void)error;

    if (strcmp(property_name, "UUID") == 0) {
        return g_variant_new_string(app->service_uuid);
    }
    if (strcmp(property_name, "Primary") == 0) {
        return g_variant_new_boolean(TRUE);
    }
    return NULL;
}

static GVariant *rx_get_property(
    GDBusConnection *connection,
    const char *sender,
    const char *object_path,
    const char *interface_name,
    const char *property_name,
    GError **error,
    gpointer user_data
) {
    ble_dbus_app_t *app = (ble_dbus_app_t *)user_data;
    static const char *const flags[] = {"write", NULL};
    unsigned char empty = 0U;

    (void)connection;
    (void)sender;
    (void)object_path;
    (void)interface_name;
    (void)error;

    if (strcmp(property_name, "UUID") == 0) {
        return g_variant_new_string(app->rx_uuid);
    }
    if (strcmp(property_name, "Service") == 0) {
        return g_variant_new_object_path(app->service_path);
    }
    if (strcmp(property_name, "Flags") == 0) {
        return new_string_array(flags);
    }
    if (strcmp(property_name, "Value") == 0) {
        return new_byte_array(&empty, 0U);
    }
    return NULL;
}

static GVariant *tx_get_property(
    GDBusConnection *connection,
    const char *sender,
    const char *object_path,
    const char *interface_name,
    const char *property_name,
    GError **error,
    gpointer user_data
) {
    ble_dbus_app_t *app = (ble_dbus_app_t *)user_data;
    static const char *const flags[] = {"notify", NULL};

    (void)connection;
    (void)sender;
    (void)object_path;
    (void)interface_name;
    (void)error;

    if (strcmp(property_name, "UUID") == 0) {
        return g_variant_new_string(app->tx_uuid);
    }
    if (strcmp(property_name, "Service") == 0) {
        return g_variant_new_object_path(app->service_path);
    }
    if (strcmp(property_name, "Flags") == 0) {
        return new_string_array(flags);
    }
    if (strcmp(property_name, "Notifying") == 0) {
        return g_variant_new_boolean(app->notify_enabled != 0);
    }
    if (strcmp(property_name, "Value") == 0) {
        return new_byte_array(app->tx_value, app->tx_len);
    }
    return NULL;
}

static GVariant *ad_get_property(
    GDBusConnection *connection,
    const char *sender,
    const char *object_path,
    const char *interface_name,
    const char *property_name,
    GError **error,
    gpointer user_data
) {
    ble_dbus_app_t *app = (ble_dbus_app_t *)user_data;
    const char *const uuids[] = {app->service_uuid, NULL};

    (void)connection;
    (void)sender;
    (void)object_path;
    (void)interface_name;
    (void)error;

    if (strcmp(property_name, "Type") == 0) {
        return g_variant_new_string("peripheral");
    }
    if (strcmp(property_name, "ServiceUUIDs") == 0) {
        return new_string_array(uuids);
    }
    if (strcmp(property_name, "LocalName") == 0) {
        return g_variant_new_string(app->device_name);
    }
    return NULL;
}

static const GDBusInterfaceVTable k_root_vtable = {
    .method_call = root_method_call,
    .get_property = NULL,
    .set_property = NULL
};

static const GDBusInterfaceVTable k_service_vtable = {
    .method_call = NULL,
    .get_property = service_get_property,
    .set_property = NULL
};

static const GDBusInterfaceVTable k_rx_vtable = {
    .method_call = rx_method_call,
    .get_property = rx_get_property,
    .set_property = NULL
};

static const GDBusInterfaceVTable k_tx_vtable = {
    .method_call = tx_method_call,
    .get_property = tx_get_property,
    .set_property = NULL
};

static const GDBusInterfaceVTable k_ad_vtable = {
    .method_call = ad_method_call,
    .get_property = ad_get_property,
    .set_property = NULL
};

static int parse_introspection(ble_dbus_app_t *app) {
    GError *error = NULL;

    app->root_node_info = g_dbus_node_info_new_for_xml(k_root_xml, &error);
    if (app->root_node_info == NULL) {
        app_log_error("root introspection parse failed: %s", error->message);
        g_clear_error(&error);
        return -1;
    }

    app->service_node_info = g_dbus_node_info_new_for_xml(k_service_xml, &error);
    if (app->service_node_info == NULL) {
        app_log_error("service introspection parse failed: %s", error->message);
        g_clear_error(&error);
        return -1;
    }

    app->rx_node_info = g_dbus_node_info_new_for_xml(k_rx_char_xml, &error);
    if (app->rx_node_info == NULL) {
        app_log_error("rx introspection parse failed: %s", error->message);
        g_clear_error(&error);
        return -1;
    }

    app->tx_node_info = g_dbus_node_info_new_for_xml(k_tx_char_xml, &error);
    if (app->tx_node_info == NULL) {
        app_log_error("tx introspection parse failed: %s", error->message);
        g_clear_error(&error);
        return -1;
    }

    app->ad_node_info = g_dbus_node_info_new_for_xml(k_ad_xml, &error);
    if (app->ad_node_info == NULL) {
        app_log_error("advertising introspection parse failed: %s", error->message);
        g_clear_error(&error);
        return -1;
    }

    return 0;
}

static int register_object(
    ble_dbus_app_t *app,
    unsigned *reg_id,
    const char *path,
    GDBusNodeInfo *node_info,
    const GDBusInterfaceVTable *vtable
) {
    GError *error = NULL;

    *reg_id = g_dbus_connection_register_object(
        APP_CONN(app),
        path,
        node_info->interfaces[0],
        vtable,
        app,
        NULL,
        &error
    );
    if (*reg_id == 0U) {
        app_log_error("register object failed for %s: %s", path, error != NULL ? error->message : "unknown");
        g_clear_error(&error);
        return -1;
    }
    return 0;
}

typedef struct {
    GMainLoop *loop;
    GVariant *reply;
    GError *error;
} bluez_call_t;

static void bluez_call_done(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    bluez_call_t *call = (bluez_call_t *)user_data;

    call->reply = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source_object), res, &call->error);
    g_main_loop_quit(call->loop);
}

static GVariant *call_bluez_with_main_context(
    ble_dbus_app_t *app,
    const char *interface_name,
    const char *method_name,
    GVariant *parameters,
    GError **error
) {
    bluez_call_t call;

    memset(&call, 0, sizeof(call));
    call.loop = g_main_loop_new(NULL, FALSE);
    if (call.loop == NULL) {
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_FAILED, "failed to create temporary main loop");
        return NULL;
    }

    g_dbus_connection_call(
        APP_CONN(app),
        "org.bluez",
        app->adapter_path,
        interface_name,
        method_name,
        parameters,
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        bluez_call_done,
        &call
    );
    g_main_loop_run(call.loop);
    g_main_loop_unref(call.loop);

    if (call.reply == NULL && error != NULL) {
        *error = call.error;
    } else {
        g_clear_error(&call.error);
    }
    return call.reply;
}

static int register_bluez_app(ble_dbus_app_t *app) {
    GError *error = NULL;
    GVariantBuilder options;
    GVariant *reply = NULL;

    g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));
    reply = call_bluez_with_main_context(
        app,
        "org.bluez.GattManager1",
        "RegisterApplication",
        g_variant_new("(o@a{sv})", app->root_path, g_variant_builder_end(&options)),
        &error
    );
    if (reply == NULL) {
        app_log_error("RegisterApplication failed: %s", error != NULL ? error->message : "unknown");
        g_clear_error(&error);
        return -1;
    }
    g_variant_unref(reply);
    return 0;
}

static int register_bluez_advertisement(ble_dbus_app_t *app) {
    GError *error = NULL;
    GVariantBuilder options;
    GVariant *reply = NULL;

    g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));
    reply = call_bluez_with_main_context(
        app,
        "org.bluez.LEAdvertisingManager1",
        "RegisterAdvertisement",
        g_variant_new("(o@a{sv})", app->ad_path, g_variant_builder_end(&options)),
        &error
    );
    if (reply == NULL) {
        app_log_error("RegisterAdvertisement failed: %s", error != NULL ? error->message : "unknown");
        g_clear_error(&error);
        return -1;
    }
    g_variant_unref(reply);
    return 0;
}

int ble_dbus_app_init(ble_dbus_app_t *app, const transport_open_args_t *args) {
    GError *error = NULL;

    if (app == NULL || args == NULL) {
        return -1;
    }

    memset(app, 0, sizeof(*app));
    snprintf(app->root_path, sizeof(app->root_path), "%s", "/com/tzs/bluetooth_sensor");
    snprintf(app->service_path, sizeof(app->service_path), "%s/service0", app->root_path);
    snprintf(app->rx_path, sizeof(app->rx_path), "%s/char_rx", app->service_path);
    snprintf(app->tx_path, sizeof(app->tx_path), "%s/char_tx", app->service_path);
    snprintf(app->ad_path, sizeof(app->ad_path), "%s/advertisement0", app->root_path);
    snprintf(app->device_name, sizeof(app->device_name), "%s", args->device_name != NULL ? args->device_name : "sensor-terminal-demo");
    snprintf(app->service_uuid, sizeof(app->service_uuid), "%s", args->service_uuid != NULL ? args->service_uuid : "");
    snprintf(app->rx_uuid, sizeof(app->rx_uuid), "%s", args->rx_uuid != NULL ? args->rx_uuid : "");
    snprintf(app->tx_uuid, sizeof(app->tx_uuid), "%s", args->tx_uuid != NULL ? args->tx_uuid : "");
    app->rx_cb = args->rx_cb;
    app->event_cb = args->event_cb;
    app->user = args->user;

    app->connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (app->connection == NULL) {
        app_log_error("connect system bus failed: %s", error != NULL ? error->message : "unknown");
        g_clear_error(&error);
        return -1;
    }

    if (find_adapter_path(app) != 0) {
        app_log_error("no BLE adapter with GATT + advertising manager found");
        return -1;
    }
    app_log_info("using adapter: %s", app->adapter_path);

    if (parse_introspection(app) != 0) {
        return -1;
    }
    if (register_object(app, &app->root_reg_id, app->root_path, APP_ROOT_NODE(app), &k_root_vtable) != 0 ||
        register_object(app, &app->service_reg_id, app->service_path, APP_SERVICE_NODE(app), &k_service_vtable) != 0 ||
        register_object(app, &app->rx_reg_id, app->rx_path, APP_RX_NODE(app), &k_rx_vtable) != 0 ||
        register_object(app, &app->tx_reg_id, app->tx_path, APP_TX_NODE(app), &k_tx_vtable) != 0 ||
        register_object(app, &app->ad_reg_id, app->ad_path, APP_AD_NODE(app), &k_ad_vtable) != 0) {
        return -1;
    }

    if (register_bluez_app(app) != 0) {
        return -1;
    }
    if (register_bluez_advertisement(app) != 0) {
        return -1;
    }

    app_log_info("BlueZ GATT app registered");
    return 0;
}

static void unregister_call(ble_dbus_app_t *app, const char *iface, const char *method, const char *path) {
    GError *error = NULL;
    GVariant *reply = NULL;

    if (APP_CONN(app) == NULL || app->adapter_path[0] == '\0') {
        return;
    }

    reply = g_dbus_connection_call_sync(
        APP_CONN(app),
        "org.bluez",
        app->adapter_path,
        iface,
        method,
        g_variant_new("(o)", path),
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );
    if (reply != NULL) {
        g_variant_unref(reply);
    } else {
        g_clear_error(&error);
    }
}

void ble_dbus_app_deinit(ble_dbus_app_t *app) {
    if (app == NULL) {
        return;
    }

    unregister_call(app, "org.bluez.LEAdvertisingManager1", "UnregisterAdvertisement", app->ad_path);
    unregister_call(app, "org.bluez.GattManager1", "UnregisterApplication", app->root_path);

    if (APP_CONN(app) != NULL) {
        if (app->root_reg_id != 0U) {
            g_dbus_connection_unregister_object(APP_CONN(app), app->root_reg_id);
        }
        if (app->service_reg_id != 0U) {
            g_dbus_connection_unregister_object(APP_CONN(app), app->service_reg_id);
        }
        if (app->rx_reg_id != 0U) {
            g_dbus_connection_unregister_object(APP_CONN(app), app->rx_reg_id);
        }
        if (app->tx_reg_id != 0U) {
            g_dbus_connection_unregister_object(APP_CONN(app), app->tx_reg_id);
        }
        if (app->ad_reg_id != 0U) {
            g_dbus_connection_unregister_object(APP_CONN(app), app->ad_reg_id);
        }
        g_object_unref(APP_CONN(app));
    }

    if (APP_ROOT_NODE(app) != NULL) {
        g_dbus_node_info_unref(APP_ROOT_NODE(app));
    }
    if (APP_SERVICE_NODE(app) != NULL) {
        g_dbus_node_info_unref(APP_SERVICE_NODE(app));
    }
    if (APP_RX_NODE(app) != NULL) {
        g_dbus_node_info_unref(APP_RX_NODE(app));
    }
    if (APP_TX_NODE(app) != NULL) {
        g_dbus_node_info_unref(APP_TX_NODE(app));
    }
    if (APP_AD_NODE(app) != NULL) {
        g_dbus_node_info_unref(APP_AD_NODE(app));
    }

    memset(app, 0, sizeof(*app));
}

int ble_dbus_app_send_notify(ble_dbus_app_t *app, const unsigned char *buf, unsigned len) {
    GVariantBuilder changed;
    GVariantBuilder invalidated;
    GError *error = NULL;

    if (app == NULL || APP_CONN(app) == NULL || buf == NULL || len == 0U || len > sizeof(app->tx_value)) {
        return -1;
    }
    if (app->notify_enabled == 0) {
        return -1;
    }

    memcpy(app->tx_value, buf, len);
    app->tx_len = len;

    g_variant_builder_init(&changed, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&changed, "{sv}", "Value", new_byte_array(app->tx_value, app->tx_len));
    g_variant_builder_init(&invalidated, G_VARIANT_TYPE("as"));
    if (!g_dbus_connection_emit_signal(
            APP_CONN(app),
            NULL,
            app->tx_path,
            "org.freedesktop.DBus.Properties",
            "PropertiesChanged",
            g_variant_new("(sa{sv}as)", "org.bluez.GattCharacteristic1", &changed, &invalidated),
            &error)) {
        app_log_error("emit notify failed: %s", error != NULL ? error->message : "unknown");
        g_clear_error(&error);
        return -1;
    }

    app_log_hex("TX", buf, len);
    return 0;
}

int ble_dbus_app_is_notify_enabled(const ble_dbus_app_t *app) {
    if (app == NULL) {
        return 0;
    }
    return app->notify_enabled;
}

#else

int ble_dbus_app_init(ble_dbus_app_t *app, const transport_open_args_t *args) {
    (void)app;
    (void)args;
    return -1;
}

void ble_dbus_app_deinit(ble_dbus_app_t *app) {
    (void)app;
}

int ble_dbus_app_send_notify(ble_dbus_app_t *app, const unsigned char *buf, unsigned len) {
    (void)app;
    (void)buf;
    (void)len;
    return -1;
}

int ble_dbus_app_is_notify_enabled(const ble_dbus_app_t *app) {
    (void)app;
    return 0;
}

#endif
