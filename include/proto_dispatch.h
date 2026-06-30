#ifndef PROTO_DISPATCH_H
#define PROTO_DISPATCH_H

int proto_dispatch_packet(
    const unsigned char *packet,
    unsigned packet_len,
    unsigned char prot,
    unsigned char *reply_buf,
    unsigned *reply_len_io
);

#endif
