#ifndef PROTO_SESSION_H
#define PROTO_SESSION_H

#include <glib.h>

#define PROTO_MAX_FRAMES 64

typedef struct {
    unsigned char active;
    unsigned char current_pseq;
    unsigned char total_frames;
    unsigned char received[PROTO_MAX_FRAMES];
    unsigned short frame_len[PROTO_MAX_FRAMES];
    unsigned char frame_data[PROTO_MAX_FRAMES][160];
    guint64 last_update_ms;
} proto_session_t;

typedef enum {
    SESSION_ACTION_ACK = 1,
    SESSION_ACTION_NACK = 2,
    SESSION_ACTION_WAIT = 3,
    SESSION_ACTION_COMPLETE = 4
} session_action_t;

void proto_session_reset(proto_session_t *session);

#endif
