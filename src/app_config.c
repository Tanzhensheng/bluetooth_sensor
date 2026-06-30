#include "app_config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void trim(char *text) {
    size_t len;
    char *start = text;

    while (*start != '\0' && isspace((unsigned char)*start)) {
        start++;
    }

    if (start != text) {
        memmove(text, start, strlen(start) + 1U);
    }

    len = strlen(text);
    while (len > 0U && isspace((unsigned char)text[len - 1U])) {
        text[len - 1U] = '\0';
        len--;
    }
}

void app_config_set_defaults(app_config_t *cfg) {
    if (cfg == NULL) {
        return;
    }

    memset(cfg, 0, sizeof(*cfg));
    snprintf(cfg->device_name, sizeof(cfg->device_name), "%s", "sensor-terminal-demo");
    snprintf(cfg->service_uuid, sizeof(cfg->service_uuid), "%s", "9b0a0001-5f3b-4b5c-8c71-000000000001");
    snprintf(cfg->rx_uuid, sizeof(cfg->rx_uuid), "%s", "9b0a0002-5f3b-4b5c-8c71-000000000001");
    snprintf(cfg->tx_uuid, sizeof(cfg->tx_uuid), "%s", "9b0a0003-5f3b-4b5c-8c71-000000000001");
    cfg->frame_timeout_ms = 3000U;
    cfg->resend_limit = 2U;
    cfg->tx_power_dbm = 5U;
    cfg->enable_repair = 1U;
}

static void apply_kv(app_config_t *cfg, const char *key, const char *value) {
    if (strcmp(key, "device_name") == 0) {
        snprintf(cfg->device_name, sizeof(cfg->device_name), "%s", value);
    } else if (strcmp(key, "service_uuid") == 0) {
        snprintf(cfg->service_uuid, sizeof(cfg->service_uuid), "%s", value);
    } else if (strcmp(key, "rx_uuid") == 0) {
        snprintf(cfg->rx_uuid, sizeof(cfg->rx_uuid), "%s", value);
    } else if (strcmp(key, "tx_uuid") == 0) {
        snprintf(cfg->tx_uuid, sizeof(cfg->tx_uuid), "%s", value);
    } else if (strcmp(key, "frame_timeout_ms") == 0) {
        cfg->frame_timeout_ms = (unsigned)strtoul(value, NULL, 10);
    } else if (strcmp(key, "resend_limit") == 0) {
        cfg->resend_limit = (unsigned)strtoul(value, NULL, 10);
    } else if (strcmp(key, "tx_power_dbm") == 0) {
        cfg->tx_power_dbm = (unsigned)strtoul(value, NULL, 10);
    } else if (strcmp(key, "enable_repair") == 0) {
        cfg->enable_repair = (unsigned)strtoul(value, NULL, 10);
    }
}

int app_config_load(const char *path, app_config_t *cfg) {
    FILE *fp;
    char line[256];

    if (path == NULL || cfg == NULL) {
        return -1;
    }

    fp = fopen(path, "r");
    if (fp == NULL) {
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *eq;
        char *key;
        char *value;

        trim(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        eq = strchr(line, '=');
        if (eq == NULL) {
            continue;
        }

        *eq = '\0';
        key = line;
        value = eq + 1;
        trim(key);
        trim(value);
        apply_kv(cfg, key, value);
    }

    fclose(fp);
    return 0;
}
