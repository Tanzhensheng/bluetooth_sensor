#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "proto_frame.h"
#include "proto_session.h"

static void assert_true(int cond, const char *msg) {
    if (!cond) {
        fprintf(stderr, "ASSERT FAIL: %s\n", msg);
        exit(1);
    }
}

static proto_frame_t make_first_frame(unsigned char pseq, unsigned char total_frames, unsigned char data0) {
    proto_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.length = 6U;
    frame.control.dir = 0U;
    frame.control.prm = 1U;
    frame.control.fun = 1U;
    frame.pseq = pseq;
    frame.fseq = (unsigned char)(0x80U | ((total_frames - 1U) & 0x3FU));
    frame.prot = 0x10U;
    frame.data[0] = data0;
    frame.data_len = 1U;
    return frame;
}

static proto_frame_t make_follow_frame(unsigned char pseq, unsigned char seq, unsigned char data0) {
    proto_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.length = 6U;
    frame.control.dir = 0U;
    frame.control.prm = 1U;
    frame.control.fun = 1U;
    frame.pseq = pseq;
    frame.fseq = seq;
    frame.prot = 0x10U;
    frame.data[0] = data0;
    frame.data_len = 1U;
    return frame;
}

extern session_action_t proto_session_accept_frame(proto_session_t *session, const proto_frame_t *frame, uint64_t now_ms);
extern unsigned proto_session_build_packet(const proto_session_t *session, unsigned char *out, unsigned out_cap);

static void test_single_frame_complete(void) {
    proto_session_t session;
    proto_frame_t frame = make_first_frame(1U, 1U, 0xAAU);
    unsigned char packet[8];

    proto_session_reset(&session);
    assert_true(proto_session_accept_frame(&session, &frame, 100U) == SESSION_ACTION_COMPLETE, "single frame completes");
    assert_true(proto_session_build_packet(&session, packet, sizeof(packet)) == 1U, "single frame packet len");
    assert_true(packet[0] == 0xAAU, "single frame payload");
}

static void test_three_frame_complete(void) {
    proto_session_t session;
    proto_frame_t f0 = make_first_frame(2U, 3U, 0x11U);
    proto_frame_t f1 = make_follow_frame(2U, 1U, 0x22U);
    proto_frame_t f2 = make_follow_frame(2U, 2U, 0x33U);
    unsigned char packet[8];

    proto_session_reset(&session);
    assert_true(proto_session_accept_frame(&session, &f0, 100U) == SESSION_ACTION_ACK, "first frame ack");
    assert_true(proto_session_accept_frame(&session, &f1, 200U) == SESSION_ACTION_ACK, "second frame ack");
    assert_true(proto_session_accept_frame(&session, &f2, 300U) == SESSION_ACTION_COMPLETE, "third frame completes");
    assert_true(proto_session_build_packet(&session, packet, sizeof(packet)) == 3U, "three-frame packet len");
    assert_true(packet[0] == 0x11U && packet[1] == 0x22U && packet[2] == 0x33U, "three-frame packet data");
}

static void test_duplicate_follow_frame_acked(void) {
    proto_session_t session;
    proto_frame_t f0 = make_first_frame(3U, 3U, 0x11U);
    proto_frame_t f1 = make_follow_frame(3U, 1U, 0x22U);

    proto_session_reset(&session);
    assert_true(proto_session_accept_frame(&session, &f0, 100U) == SESSION_ACTION_ACK, "first frame ack");
    assert_true(proto_session_accept_frame(&session, &f1, 200U) == SESSION_ACTION_ACK, "first follow ack");
    assert_true(proto_session_accept_frame(&session, &f1, 250U) == SESSION_ACTION_ACK, "duplicate follow ack");
}

static void test_timeout_resets_session(void) {
    proto_session_t session;
    proto_frame_t oldf = make_first_frame(4U, 3U, 0x11U);
    proto_frame_t newf = make_first_frame(5U, 1U, 0x66U);
    unsigned char packet[8];

    proto_session_reset(&session);
    assert_true(proto_session_accept_frame(&session, &oldf, 100U) == SESSION_ACTION_ACK, "old first frame ack");
    assert_true(proto_session_accept_frame(&session, &newf, 4101U) == SESSION_ACTION_COMPLETE, "timeout allows new frame");
    assert_true(proto_session_build_packet(&session, packet, sizeof(packet)) == 1U, "timeout packet len");
    assert_true(packet[0] == 0x66U, "timeout replaced payload");
}

int main(void) {
    test_single_frame_complete();
    test_three_frame_complete();
    test_duplicate_follow_frame_acked();
    test_timeout_resets_session();
    puts("test_proto_session: PASS");
    return 0;
}
