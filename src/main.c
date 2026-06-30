#include <glib.h>
#include <stdio.h>

#include "app_config.h"
#include "app_log.h"
#include "transport_ble_gatt.h"
#include "transport_socket.h"

int main(int argc, char **argv) {
    GMainLoop *loop = NULL;
    app_config_t cfg;
    const char *cfg_path = "conf/sensor_terminal.ini";

    if (argc > 1) {
        cfg_path = argv[1];
    }

    app_config_set_defaults(&cfg);
    if (app_config_load(cfg_path, &cfg) != 0) {
        app_log_warn("failed to load config: %s, using defaults", cfg_path);
    }

    transport_socket_set_backend(transport_ble_gatt_backend());

    app_log_info(
        "device=%s timeout=%u resend=%u repair=%u",
        cfg.device_name,
        cfg.frame_timeout_ms,
        cfg.resend_limit,
        cfg.enable_repair
    );

    loop = g_main_loop_new(NULL, FALSE);
    if (loop == NULL) {
        app_log_error("failed to create main loop");
        return 1;
    }

    app_log_info("sensor terminal skeleton started");
    g_main_loop_run(loop);
    g_main_loop_unref(loop);
    return 0;
}
