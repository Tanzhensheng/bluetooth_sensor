#ifndef BLE_DBUS_APP_H
#define BLE_DBUS_APP_H

#include "transport_api.h"

typedef struct {
    void *connection;
    void *root_node_info;
    void *service_node_info;
    void *rx_node_info;
    void *tx_node_info;
    void *ad_node_info;
    unsigned root_reg_id;
    unsigned service_reg_id;
    unsigned rx_reg_id;
    unsigned tx_reg_id;
    unsigned ad_reg_id;
    char adapter_path[128];
    char root_path[64];
    char service_path[96];
    char rx_path[128];
    char tx_path[128];
    char ad_path[96];
    char device_name[64];
    char service_uuid[40];
    char rx_uuid[40];
    char tx_uuid[40];
    unsigned char tx_value[192];
    unsigned tx_len;
    int notify_enabled;
    transport_rx_cb rx_cb;
    transport_event_cb event_cb;
    void *user;
} ble_dbus_app_t;

int ble_dbus_app_init(ble_dbus_app_t *app, const transport_open_args_t *args);
void ble_dbus_app_deinit(ble_dbus_app_t *app);
int ble_dbus_app_send_notify(ble_dbus_app_t *app, const unsigned char *buf, unsigned len);
int ble_dbus_app_is_notify_enabled(const ble_dbus_app_t *app);

#endif
