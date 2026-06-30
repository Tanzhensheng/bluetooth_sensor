#include "proto_session.h"

#include <string.h>

static unsigned frame_index(const proto_frame_t *frame) {
    if ((frame->fseq & 0x80U) != 0U) {
        return 0U;
    }
    return (unsigned)(frame->fseq & 0x3FU);
}

static unsigned frame_total(const proto_frame_t *frame) {
    return (unsigned)(frame->fseq & 0x3FU) + 1U;
}

static int session_is_complete(const proto_session_t *session) {
    unsigned i;
    for (i = 0; i < session->total_frames; ++i) {
        if (session->received[i] == 0U) {
            return 0;
        }
    }
    return 1;
}

static void store_frame(proto_session_t *session, unsigned index, const proto_frame_t *frame, uint64_t now_ms) {
    session->received[index] = 1U;
    session->frame_len[index] = frame->data_len;
    if (frame->data_len > 0U) {
        memcpy(session->frame_data[index], frame->data, frame->data_len);
    }
    session->last_update_ms = now_ms;
}

void proto_session_reset(proto_session_t *session) {
    if (session == NULL) {
        return;
    }
    memset(session, 0, sizeof(*session));
}

session_action_t proto_session_accept_frame(proto_session_t *session, const proto_frame_t *frame, uint64_t now_ms) {
    unsigned index;
    unsigned total;

    if (session == NULL || frame == NULL) {
        return SESSION_ACTION_NACK;
    }

    if (session->active != 0U && now_ms > session->last_update_ms &&
        (now_ms - session->last_update_ms) > PROTO_SESSION_TIMEOUT_MS) {
        proto_session_reset(session);
    }

    if ((frame->fseq & 0x80U) != 0U) {
        total = frame_total(frame);
        if (total == 0U || total > PROTO_MAX_FRAMES) {
            return SESSION_ACTION_NACK;
        }

        if (session->active != 0U && session->current_pseq != frame->pseq) {
            return SESSION_ACTION_NACK;
        }

        if (session->active == 0U) {
            proto_session_reset(session);
            session->active = 1U;
            session->current_pseq = frame->pseq;
            session->total_frames = (unsigned char)total;
        }

        if (session->received[0] != 0U) {
            return session_is_complete(session) ? SESSION_ACTION_COMPLETE : SESSION_ACTION_ACK;
        }

        store_frame(session, 0U, frame, now_ms);
        return (total == 1U) ? SESSION_ACTION_COMPLETE : SESSION_ACTION_ACK;
    }

    if (session->active == 0U || session->current_pseq != frame->pseq) {
        return SESSION_ACTION_NACK;
    }

    index = frame_index(frame);
    if (index == 0U || index >= session->total_frames) {
        return SESSION_ACTION_NACK;
    }

    if (session->received[index] != 0U) {
        return session_is_complete(session) ? SESSION_ACTION_COMPLETE : SESSION_ACTION_ACK;
    }

    store_frame(session, index, frame, now_ms);
    return session_is_complete(session) ? SESSION_ACTION_COMPLETE : SESSION_ACTION_ACK;
}

unsigned proto_session_build_packet(const proto_session_t *session, unsigned char *out, unsigned out_cap) {
    unsigned i;
    unsigned offset = 0U;

    if (session == NULL || out == NULL || session_is_complete(session) == 0) {
        return 0U;
    }

    for (i = 0; i < session->total_frames; ++i) {
        if (offset + session->frame_len[i] > out_cap) {
            return 0U;
        }
        if (session->frame_len[i] > 0U) {
            memcpy(&out[offset], session->frame_data[i], session->frame_len[i]);
            offset += session->frame_len[i];
        }
    }

    return offset;
}
