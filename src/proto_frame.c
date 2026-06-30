#include "proto_frame.h"

#include <string.h>

static unsigned char encode_control(const proto_control_t *control) {
    return (unsigned char)(
        ((control->dir & 0x01U) << 7U) |
        ((control->prm & 0x01U) << 6U) |
        ((control->fcb & 0x01U) << 5U) |
        ((control->fcv & 0x01U) << 4U) |
        (control->fun & 0x0FU)
    );
}

static void decode_control(unsigned char value, proto_control_t *control) {
    control->dir = (unsigned char)((value >> 7U) & 0x01U);
    control->prm = (unsigned char)((value >> 6U) & 0x01U);
    control->fcb = (unsigned char)((value >> 5U) & 0x01U);
    control->fcv = (unsigned char)((value >> 4U) & 0x01U);
    control->fun = (unsigned char)(value & 0x0FU);
}

int proto_frame_decode(const unsigned char *buf, unsigned len, proto_frame_t *out) {
    unsigned short payload_len;
    unsigned expected_len;
    unsigned char cs;

    if (buf == NULL || out == NULL || len < 9U) {
        return -1;
    }

    if (buf[0] != 0xA5U || buf[len - 1U] != 0x96U) {
        return -1;
    }

    payload_len = (unsigned short)(buf[1] | ((unsigned short)buf[2] << 8U));
    if (payload_len < 4U) {
        return -1;
    }

    expected_len = (unsigned)payload_len + 5U;
    if (len != expected_len) {
        return -1;
    }

    cs = proto_frame_calc_cs(&buf[3], payload_len);
    if (cs != buf[len - 2U]) {
        return -1;
    }

    memset(out, 0, sizeof(*out));
    out->length = payload_len;
    decode_control(buf[3], &out->control);
    out->pseq = buf[4];
    out->fseq = buf[5];
    out->prot = buf[6];
    out->data_len = (unsigned short)(payload_len - 4U);
    if (out->data_len > sizeof(out->data)) {
        return -1;
    }
    if (out->data_len > 0U) {
        memcpy(out->data, &buf[7], out->data_len);
    }
    return 0;
}

int proto_frame_encode(const proto_frame_t *frame, unsigned char *buf, unsigned *len_io) {
    unsigned payload_len;
    unsigned total_len;

    if (frame == NULL || buf == NULL || len_io == NULL) {
        return -1;
    }

    if (frame->data_len > sizeof(frame->data)) {
        return -1;
    }

    payload_len = 4U + frame->data_len;
    total_len = payload_len + 5U;
    if (*len_io < total_len) {
        return -1;
    }

    buf[0] = 0xA5U;
    buf[1] = (unsigned char)(payload_len & 0xFFU);
    buf[2] = (unsigned char)((payload_len >> 8U) & 0xFFU);
    buf[3] = encode_control(&frame->control);
    buf[4] = frame->pseq;
    buf[5] = frame->fseq;
    buf[6] = frame->prot;
    if (frame->data_len > 0U) {
        memcpy(&buf[7], frame->data, frame->data_len);
    }
    buf[7U + frame->data_len] = proto_frame_calc_cs(&buf[3], payload_len);
    buf[8U + frame->data_len] = 0x96U;
    *len_io = total_len;
    return 0;
}

unsigned char proto_frame_calc_cs(const unsigned char *buf, unsigned len) {
    unsigned char sum = 0U;
    unsigned i;
    for (i = 0; i < len; ++i) {
        sum = (unsigned char)(sum + buf[i]);
    }
    return sum;
}
