#ifndef APP_LOG_H
#define APP_LOG_H

void app_log_info(const char *fmt, ...);
void app_log_warn(const char *fmt, ...);
void app_log_error(const char *fmt, ...);
void app_log_hex(const char *tag, const unsigned char *buf, unsigned len);

#endif
