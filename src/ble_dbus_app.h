#ifndef BLE_DBUS_APP_H
#define BLE_DBUS_APP_H

typedef struct {
    int placeholder;
} ble_dbus_app_t;

int ble_dbus_app_init(ble_dbus_app_t *app);
void ble_dbus_app_deinit(ble_dbus_app_t *app);

#endif
