#include "transport_ble_gatt.h"

struct transport_handle {
    transport_open_args_t args;
};

static int ble_gatt_open(transport_handle_t **handle, const transport_open_args_t *args) {
    (void)handle;
    (void)args;
    return -1;
}

static int ble_gatt_close(transport_handle_t *handle) {
    (void)handle;
    return -1;
}

static int ble_gatt_send(transport_handle_t *handle, const unsigned char *buf, unsigned len) {
    (void)handle;
    (void)buf;
    (void)len;
    return -1;
}

static int ble_gatt_poll(transport_handle_t *handle, int timeout_ms) {
    (void)handle;
    (void)timeout_ms;
    return -1;
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
