#include "proto_frame.h"

int proto_frame_decode(const unsigned char *buf, unsigned len, proto_frame_t *out) {
    (void)buf;
    (void)len;
    (void)out;
    return -1;
}

int proto_frame_encode(const proto_frame_t *frame, unsigned char *buf, unsigned *len_io) {
    (void)frame;
    (void)buf;
    (void)len_io;
    return -1;
}

unsigned char proto_frame_calc_cs(const unsigned char *buf, unsigned len) {
    unsigned char sum = 0U;
    unsigned i;
    for (i = 0; i < len; ++i) {
        sum = (unsigned char)(sum + buf[i]);
    }
    return sum;
}
