#include "proto_dispatch.h"

int proto_dispatch_packet(
    const unsigned char *packet,
    unsigned packet_len,
    unsigned char prot,
    unsigned char *reply_buf,
    unsigned *reply_len_io
) {
    (void)packet;
    (void)packet_len;
    (void)prot;
    (void)reply_buf;
    (void)reply_len_io;
    return -1;
}
