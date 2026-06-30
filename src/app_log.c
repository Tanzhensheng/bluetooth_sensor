#include "app_log.h"

#include <stdarg.h>
#include <stdio.h>

static void app_log_vprint(const char *level, const char *fmt, va_list args) {
    fprintf(stderr, "%s ", level);
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
}

void app_log_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    app_log_vprint("INFO", fmt, args);
    va_end(args);
}

void app_log_warn(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    app_log_vprint("WARN", fmt, args);
    va_end(args);
}

void app_log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    app_log_vprint("ERROR", fmt, args);
    va_end(args);
}

void app_log_hex(const char *tag, const unsigned char *buf, unsigned len) {
    unsigned i;

    if (tag == NULL) {
        tag = "HEX";
    }

    fprintf(stderr, "%s", tag);
    for (i = 0; i < len; ++i) {
        fprintf(stderr, " %02X", buf[i]);
    }
    fputc('\n', stderr);
}
