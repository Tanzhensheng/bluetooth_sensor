#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "transport_api.h"
#include "transport_ble_gatt.h"
#include "transport_socket.h"

static void assert_true(int cond, const char *msg) {
    if (!cond) {
        fprintf(stderr, "ASSERT FAIL: %s\n", msg);
        exit(1);
    }
}

static void test_transport_requires_backend(void) {
    transport_handle_t *handle = NULL;
    transport_open_args_t args;

    memset(&args, 0, sizeof(args));
    transport_socket_set_backend(NULL);
    assert_true(transport_open(&handle, &args) != 0, "open without backend fails");
}

static void test_transport_open_send_close_with_ble_backend(void) {
    transport_handle_t *handle = NULL;
    transport_open_args_t args;
    const unsigned char payload[] = {0x11, 0x22, 0x33};

    memset(&args, 0, sizeof(args));
    args.device_name = "sensor-terminal-demo";
    args.service_uuid = "svc";
    args.rx_uuid = "rx";
    args.tx_uuid = "tx";

    transport_socket_set_backend(transport_ble_gatt_backend());
    assert_true(transport_open(&handle, &args) == 0, "open with backend");
    assert_true(handle != NULL, "handle assigned");
    assert_true(transport_send(handle, payload, sizeof(payload)) == 0, "send works");
    assert_true(transport_poll(handle, 0) == 0, "poll works");
    assert_true(transport_close(handle) == 0, "close works");
}

int main(void) {
    test_transport_requires_backend();
    test_transport_open_send_close_with_ble_backend();
    puts("test_transport_api: PASS");
    return 0;
}
