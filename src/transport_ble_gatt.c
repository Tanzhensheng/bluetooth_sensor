#include "transport_ble_gatt.h"

#include <stdlib.h>
#include <string.h>

#include "tx_queue.h"

struct transport_handle {
    transport_open_args_t args;
    int opened;
};

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
    *handle = inst;
    return 0;
}

static int ble_gatt_close(transport_handle_t *handle) {
    if (handle == NULL) {
        return -1;
    }
    handle->opened = 0;
    free(handle);
    return 0;
}

static int ble_gatt_send(transport_handle_t *handle, const unsigned char *buf, unsigned len) {
    if (handle == NULL || handle->opened == 0 || buf == NULL || len == 0U) {
        return -1;
    }
    return tx_queue_push(buf, len);
}

static int ble_gatt_poll(transport_handle_t *handle, int timeout_ms) {
    if (handle == NULL || handle->opened == 0) {
        return -1;
    }
    (void)timeout_ms;
    return 0;
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
