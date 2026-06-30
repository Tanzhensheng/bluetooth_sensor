#include "tx_queue.h"

#include <string.h>

static tx_item_t g_item;
static int g_has_item;

void tx_queue_reset(void) {
    memset(&g_item, 0, sizeof(g_item));
    g_has_item = 0;
}

int tx_queue_push(const unsigned char *buf, unsigned len) {
    if (buf == NULL || len > sizeof(g_item.bytes)) {
        return -1;
    }
    memcpy(g_item.bytes, buf, len);
    g_item.len = len;
    g_has_item = 1;
    return 0;
}

int tx_queue_pop(unsigned char *buf, unsigned *len_io) {
    if (!g_has_item || buf == NULL || len_io == NULL || *len_io < g_item.len) {
        return -1;
    }
    memcpy(buf, g_item.bytes, g_item.len);
    *len_io = g_item.len;
    g_has_item = 0;
    return 0;
}
