#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "proto_dispatch.h"

static void assert_true(int cond, const char *msg) {
    if (!cond) {
        fprintf(stderr, "ASSERT FAIL: %s\n", msg);
        exit(1);
    }
}

static int contains_text(const unsigned char *buf, unsigned len, const char *text) {
    unsigned text_len = (unsigned)strlen(text);
    unsigned i;

    if (text_len == 0U || len < text_len) {
        return 0;
    }

    for (i = 0; i + text_len <= len; ++i) {
        if (memcmp(&buf[i], text, text_len) == 0) {
            return 1;
        }
    }
    return 0;
}

static void test_prot_10_returns_sensor_reply(void) {
    unsigned char reply[128];
    unsigned reply_len = sizeof(reply);
    const unsigned char packet[] = {0x01, 0x02, 0x03};

    assert_true(proto_dispatch_packet(packet, sizeof(packet), 0x10U, reply, &reply_len) == 0, "prot 10 dispatch ok");
    assert_true(reply_len > 0U, "prot 10 reply len");
    assert_true(contains_text(reply, reply_len, "sensor-terminal ok") != 0, "prot 10 reply text");
}

static void test_prot_00_returns_not_implemented(void) {
    unsigned char reply[128];
    unsigned reply_len = sizeof(reply);

    assert_true(proto_dispatch_packet(NULL, 0U, 0x00U, reply, &reply_len) == 0, "prot 00 dispatch ok");
    assert_true(contains_text(reply, reply_len, "not-implemented") != 0, "prot 00 reply text");
}

static void test_unknown_protocol_rejected(void) {
    unsigned char reply[128];
    unsigned reply_len = sizeof(reply);

    assert_true(proto_dispatch_packet(NULL, 0U, 0x77U, reply, &reply_len) != 0, "unknown protocol rejected");
}

int main(void) {
    test_prot_10_returns_sensor_reply();
    test_prot_00_returns_not_implemented();
    test_unknown_protocol_rejected();
    puts("test_proto_dispatch: PASS");
    return 0;
}
