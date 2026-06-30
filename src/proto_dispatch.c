#include "proto_dispatch.h"

#include <string.h>

int proto_dispatch_packet(
    const unsigned char *packet,
    unsigned packet_len,
    unsigned char prot,
    unsigned char *reply_buf,
    unsigned *reply_len_io
) {
    const char *reply_text = NULL;
    unsigned reply_len;

    (void)packet;
    (void)packet_len;

    if (reply_buf == NULL || reply_len_io == NULL) {
        return -1;
    }

    if (prot == 0x10U) {
        reply_text = "sensor-terminal ok";
    } else if (prot == 0x00U || prot == 0x01U) {
        reply_text = "not-implemented";
    } else {
        return -1;
    }

    reply_len = (unsigned)strlen(reply_text);
    if (*reply_len_io < reply_len) {
        return -1;
    }

    memcpy(reply_buf, reply_text, reply_len);
    *reply_len_io = reply_len;
    return 0;
}
