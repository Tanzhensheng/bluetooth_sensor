#ifndef TX_QUEUE_H
#define TX_QUEUE_H

typedef struct tx_item {
    unsigned len;
    unsigned char bytes[192];
    struct tx_item *next;
} tx_item_t;

void tx_queue_reset(void);
int tx_queue_push(const unsigned char *buf, unsigned len);
int tx_queue_pop(unsigned char *buf, unsigned *len_io);

#endif
