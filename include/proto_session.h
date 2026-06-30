#ifndef PROTO_SESSION_H
#define PROTO_SESSION_H

#include <stdint.h>

#include "proto_frame.h"

#define PROTO_MAX_FRAMES 64
#define PROTO_SESSION_TIMEOUT_MS 3000U

typedef struct {
    unsigned char active;
    unsigned char current_pseq;
    unsigned char total_frames;
    unsigned char received[PROTO_MAX_FRAMES];
    unsigned short frame_len[PROTO_MAX_FRAMES];
    unsigned char frame_data[PROTO_MAX_FRAMES][160];
    uint64_t last_update_ms;
} proto_session_t;

typedef enum {
    SESSION_ACTION_ACK = 1,
    SESSION_ACTION_NACK = 2,
    SESSION_ACTION_WAIT = 3,
    SESSION_ACTION_COMPLETE = 4
} session_action_t;

void proto_session_reset(proto_session_t *session);
session_action_t proto_session_accept_frame(proto_session_t *session, const proto_frame_t *frame, uint64_t now_ms);
unsigned proto_session_build_packet(const proto_session_t *session, unsigned char *out, unsigned out_cap);

#endif
