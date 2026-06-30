#ifndef PROTO_FRAME_H
#define PROTO_FRAME_H

typedef struct {
    unsigned char dir;
    unsigned char prm;
    unsigned char fcb;
    unsigned char fcv;
    unsigned char fun;
} proto_control_t;

typedef struct {
    unsigned short length;
    proto_control_t control;
    unsigned char pseq;
    unsigned char fseq;
    unsigned char prot;
    unsigned char data[160];
    unsigned short data_len;
} proto_frame_t;

int proto_frame_decode(const unsigned char *buf, unsigned len, proto_frame_t *out);
int proto_frame_encode(const proto_frame_t *frame, unsigned char *buf, unsigned *len_io);
unsigned char proto_frame_calc_cs(const unsigned char *buf, unsigned len);

#endif
