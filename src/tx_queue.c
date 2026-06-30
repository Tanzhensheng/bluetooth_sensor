#include "tx_queue.h"

#include <stdlib.h>
#include <string.h>

static tx_item_t *g_head;
static tx_item_t *g_tail;

void tx_queue_reset(void) {
    tx_item_t *item = g_head;
    while (item != NULL) {
        tx_item_t *next = item->next;
        free(item);
        item = next;
    }
    g_head = NULL;
    g_tail = NULL;
}

int tx_queue_push(const unsigned char *buf, unsigned len) {
    tx_item_t *item;

    if (buf == NULL || len > sizeof(((tx_item_t *)0)->bytes)) {
        return -1;
    }

    item = (tx_item_t *)calloc(1, sizeof(*item));
    if (item == NULL) {
        return -1;
    }

    memcpy(item->bytes, buf, len);
    item->len = len;
    item->next = NULL;

    if (g_tail == NULL) {
        g_head = item;
        g_tail = item;
    } else {
        g_tail->next = item;
        g_tail = item;
    }
    return 0;
}

int tx_queue_pop(unsigned char *buf, unsigned *len_io) {
    tx_item_t *item;

    if (g_head == NULL || buf == NULL || len_io == NULL) {
        return -1;
    }

    item = g_head;
    if (*len_io < item->len) {
        return -1;
    }

    memcpy(buf, item->bytes, item->len);
    *len_io = item->len;

    g_head = item->next;
    if (g_head == NULL) {
        g_tail = NULL;
    }
    free(item);
    return 0;
}
