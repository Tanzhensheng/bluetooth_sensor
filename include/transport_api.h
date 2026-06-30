#ifndef TRANSPORT_API_H
#define TRANSPORT_API_H

typedef struct transport_handle transport_handle_t;

typedef enum {
    TRANSPORT_EVENT_CONNECTED = 1,
    TRANSPORT_EVENT_DISCONNECTED = 2,
    TRANSPORT_EVENT_NOTIFY_ENABLED = 3,
    TRANSPORT_EVENT_NOTIFY_DISABLED = 4
} transport_event_t;

typedef void (*transport_rx_cb)(const unsigned char *buf, unsigned len, void *user);
typedef void (*transport_event_cb)(int event, void *user);

typedef struct {
    const char *device_name;
    const char *service_uuid;
    const char *rx_uuid;
    const char *tx_uuid;
    transport_rx_cb rx_cb;
    transport_event_cb event_cb;
    void *user;
} transport_open_args_t;

int transport_open(transport_handle_t **handle, const transport_open_args_t *args);
int transport_close(transport_handle_t *handle);
int transport_send(transport_handle_t *handle, const unsigned char *buf, unsigned len);
int transport_poll(transport_handle_t *handle, int timeout_ms);

#endif
