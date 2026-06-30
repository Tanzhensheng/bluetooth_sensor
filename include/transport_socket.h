#ifndef TRANSPORT_SOCKET_H
#define TRANSPORT_SOCKET_H

#include "transport_api.h"

typedef struct {
    int (*open)(transport_handle_t **handle, const transport_open_args_t *args);
    int (*close)(transport_handle_t *handle);
    int (*send)(transport_handle_t *handle, const unsigned char *buf, unsigned len);
    int (*poll)(transport_handle_t *handle, int timeout_ms);
} transport_vtable_t;

void transport_socket_set_backend(const transport_vtable_t *backend);
const transport_vtable_t *transport_socket_get_backend(void);

#endif
