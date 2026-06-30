#ifndef APP_CONFIG_H
#define APP_CONFIG_H

typedef struct {
    char device_name[64];
    char service_uuid[40];
    char rx_uuid[40];
    char tx_uuid[40];
    unsigned frame_timeout_ms;
    unsigned resend_limit;
    unsigned tx_power_dbm;
    unsigned enable_repair;
} app_config_t;

int app_config_load(const char *path, app_config_t *cfg);
void app_config_set_defaults(app_config_t *cfg);

#endif
