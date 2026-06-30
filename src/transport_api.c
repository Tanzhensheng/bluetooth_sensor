#include "transport_api.h"

#include "transport_socket.h"

int transport_open(transport_handle_t **handle, const transport_open_args_t *args) {
    const transport_vtable_t *backend = transport_socket_get_backend();
    if (backend == NULL || backend->open == NULL) {
        return -1;
    }
    return backend->open(handle, args);
}

int transport_close(transport_handle_t *handle) {
    const transport_vtable_t *backend = transport_socket_get_backend();
    if (backend == NULL || backend->close == NULL) {
        return -1;
    }
    return backend->close(handle);
}

int transport_send(transport_handle_t *handle, const unsigned char *buf, unsigned len) {
    const transport_vtable_t *backend = transport_socket_get_backend();
    if (backend == NULL || backend->send == NULL) {
        return -1;
    }
    return backend->send(handle, buf, len);
}

int transport_poll(transport_handle_t *handle, int timeout_ms) {
    const transport_vtable_t *backend = transport_socket_get_backend();
    if (backend == NULL || backend->poll == NULL) {
        return -1;
    }
    return backend->poll(handle, timeout_ms);
}
