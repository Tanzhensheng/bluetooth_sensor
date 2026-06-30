#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tx_queue.h"

static void assert_true(int cond, const char *msg) {
    if (!cond) {
        fprintf(stderr, "ASSERT FAIL: %s\n", msg);
        exit(1);
    }
}

static void test_push_two_pop_two_fifo(void) {
    unsigned char out[32];
    unsigned len;
    const unsigned char a[] = {0x01, 0x02};
    const unsigned char b[] = {0x03, 0x04, 0x05};

    tx_queue_reset();
    assert_true(tx_queue_push(a, sizeof(a)) == 0, "push first");
    assert_true(tx_queue_push(b, sizeof(b)) == 0, "push second");

    len = sizeof(out);
    assert_true(tx_queue_pop(out, &len) == 0, "pop first");
    assert_true(len == sizeof(a), "first len");
    assert_true(memcmp(out, a, sizeof(a)) == 0, "first bytes");

    len = sizeof(out);
    assert_true(tx_queue_pop(out, &len) == 0, "pop second");
    assert_true(len == sizeof(b), "second len");
    assert_true(memcmp(out, b, sizeof(b)) == 0, "second bytes");
}

int main(void) {
    test_push_two_pop_two_fifo();
    puts("test_tx_queue: PASS");
    return 0;
}
