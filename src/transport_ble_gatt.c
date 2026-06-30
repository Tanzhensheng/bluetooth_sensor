#include "transport_ble_gatt.h"

#include <stdlib.h>
#include <string.h>

#include "app_log.h"
#include "ble_dbus_app.h"
#include "tx_queue.h"

struct transport_handle {
    transport_open_args_t args;
    int opened;
    ble_dbus_app_t app;
};

#ifdef __linux__
static int ble_gatt_flush_queue(transport_handle_t *handle) {
    unsigned char buf[192];
    unsigned len = sizeof(buf);

    while (ble_dbus_app_is_notify_enabled(&handle->app) != 0) {
        len = sizeof(buf);
        if (tx_queue_pop(buf, &len) != 0) {
            break;
        }
        if (ble_dbus_app_send_notify(&handle->app, buf, len) != 0) {
            return -1;
        }
    }
    return 0;
}
#endif

static int ble_gatt_open(transport_handle_t **handle, const transport_open_args_t *args) {
    transport_handle_t *inst;

    if (handle == NULL || args == NULL) {
        return -1;
    }

    inst = (transport_handle_t *)calloc(1, sizeof(*inst));
    if (inst == NULL) {
        return -1;
    }

    inst->args = *args;
    inst->opened = 1;
    tx_queue_reset();
#ifdef __linux__
    if (ble_dbus_app_init(&inst->app, args) != 0) {
        free(inst);
        return -1;
    }
#endif
    *handle = inst;
    return 0;
}

static int ble_gatt_close(transport_handle_t *handle) {
    if (handle == NULL) {
        return -1;
    }
#ifdef __linux__
    ble_dbus_app_deinit(&handle->app);
#endif
    handle->opened = 0;
    free(handle);
    return 0;
}

static int ble_gatt_send(transport_handle_t *handle, const unsigned char *buf, unsigned len) {
    if (handle == NULL || handle->opened == 0 || buf == NULL || len == 0U) {
        return -1;
    }
    if (tx_queue_push(buf, len) != 0) {
        return -1;
    }
#ifdef __linux__
    if (ble_dbus_app_is_notify_enabled(&handle->app) != 0) {
        return ble_gatt_flush_queue(handle);
    }
#endif
    return 0;
}

static int ble_gatt_poll(transport_handle_t *handle, int timeout_ms) {
    if (handle == NULL || handle->opened == 0) {
        return -1;
    }
    (void)timeout_ms;
#ifdef __linux__
    return ble_gatt_flush_queue(handle);
#else
    return 0;
#endif
}

const transport_vtable_t *transport_ble_gatt_backend(void) {
    static const transport_vtable_t backend = {
        ble_gatt_open,
        ble_gatt_close,
        ble_gatt_send,
        ble_gatt_poll
    };
    return &backend;
}
