#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "proto_frame.h"

static void assert_true(int cond, const char *msg) {
    if (!cond) {
        fprintf(stderr, "ASSERT FAIL: %s\n", msg);
        exit(1);
    }
}

static void test_decode_valid_single_frame(void) {
    unsigned char raw[] = {0xA5, 0x05, 0x00, 0x51, 0x01, 0x80, 0x10, 0x00, 0xE2, 0x96};
    proto_frame_t frame;

    memset(&frame, 0, sizeof(frame));
    assert_true(proto_frame_decode(raw, sizeof(raw), &frame) == 0, "decode valid frame");
    assert_true(frame.length == 5U, "length parsed");
    assert_true(frame.control.dir == 0U, "dir parsed");
    assert_true(frame.control.prm == 1U, "prm parsed");
    assert_true(frame.control.fun == 1U, "fun parsed");
    assert_true(frame.pseq == 0x01U, "pseq parsed");
    assert_true(frame.fseq == 0x80U, "fseq parsed");
    assert_true(frame.prot == 0x10U, "prot parsed");
    assert_true(frame.data_len == 1U, "data len parsed");
    assert_true(frame.data[0] == 0x00U, "data payload parsed");
}

static void test_encode_round_trip(void) {
    unsigned char raw[] = {0xA5, 0x05, 0x00, 0x51, 0x01, 0x80, 0x10, 0x00, 0xE2, 0x96};
    unsigned char out[32];
    unsigned out_len = sizeof(out);
    proto_frame_t frame;

    memset(&frame, 0, sizeof(frame));
    assert_true(proto_frame_decode(raw, sizeof(raw), &frame) == 0, "decode for roundtrip");
    assert_true(proto_frame_encode(&frame, out, &out_len) == 0, "encode roundtrip");
    assert_true(out_len == sizeof(raw), "encoded length");
    assert_true(memcmp(out, raw, sizeof(raw)) == 0, "encoded bytes match");
}

static void test_reject_bad_checksum(void) {
    unsigned char raw[] = {0xA5, 0x05, 0x00, 0x51, 0x01, 0x80, 0x10, 0x00, 0xE3, 0x96};
    proto_frame_t frame;

    memset(&frame, 0, sizeof(frame));
    assert_true(proto_frame_decode(raw, sizeof(raw), &frame) != 0, "bad checksum rejected");
}

int main(void) {
    test_decode_valid_single_frame();
    test_encode_round_trip();
    test_reject_bad_checksum();
    puts("test_proto_frame: PASS");
    return 0;
}
