#include "transport_socket.h"

static const transport_vtable_t *g_backend;

void transport_socket_set_backend(const transport_vtable_t *backend) {
    g_backend = backend;
}

const transport_vtable_t *transport_socket_get_backend(void) {
    return g_backend;
}
