#include <glib.h>
#include <string.h>

#include "app_config.h"
#include "app_log.h"
#include "proto_dispatch.h"
#include "proto_frame.h"
#include "proto_session.h"
#include "transport_api.h"
#include "transport_ble_gatt.h"
#include "transport_socket.h"

typedef struct {
    GMainLoop *loop;
    transport_handle_t *transport;
    proto_session_t session;
} app_ctx_t;

static int app_send_frame(app_ctx_t *ctx, const proto_frame_t *frame) {
    unsigned char encoded[192];
    unsigned encoded_len = sizeof(encoded);

    if (ctx == NULL || ctx->transport == NULL || frame == NULL) {
        return -1;
    }
    if (proto_frame_encode(frame, encoded, &encoded_len) != 0) {
        return -1;
    }
    return transport_send(ctx->transport, encoded, encoded_len);
}

static void app_fill_ack_frame(proto_frame_t *reply, const proto_frame_t *request, int nack) {
    memset(reply, 0, sizeof(*reply));
    reply->control.dir = 1U;
    reply->control.prm = 0U;
    reply->control.fun = (unsigned char)(nack != 0 ? 1U : 0U);
    reply->pseq = request->pseq;
    reply->fseq = 0x80U;
    reply->prot = request->prot;
    reply->data_len = 0U;
}

static int app_send_ack(app_ctx_t *ctx, const proto_frame_t *request, int nack) {
    proto_frame_t reply;
    app_fill_ack_frame(&reply, request, nack);
    return app_send_frame(ctx, &reply);
}

static int app_send_business_reply(app_ctx_t *ctx, const proto_frame_t *request, const unsigned char *payload, unsigned payload_len) {
    proto_frame_t reply;

    if (payload == NULL || payload_len > sizeof(reply.data)) {
        return -1;
    }

    memset(&reply, 0, sizeof(reply));
    reply.control.dir = 1U;
    reply.control.prm = 1U;
    reply.control.fun = 1U;
    reply.pseq = request->pseq;
    reply.fseq = 0x80U;
    reply.prot = request->prot;
    reply.data_len = (unsigned short)payload_len;
    if (payload_len > 0U) {
        memcpy(reply.data, payload, payload_len);
    }
    return app_send_frame(ctx, &reply);
}

static void app_on_transport_event(int event, void *user) {
    app_ctx_t *ctx = (app_ctx_t *)user;

    (void)ctx;
    if (event == TRANSPORT_EVENT_NOTIFY_ENABLED) {
        app_log_info("TX notify enabled");
    } else if (event == TRANSPORT_EVENT_NOTIFY_DISABLED) {
        app_log_info("TX notify disabled");
    } else if (event == TRANSPORT_EVENT_CONNECTED) {
        app_log_info("BLE peer connected");
    } else if (event == TRANSPORT_EVENT_DISCONNECTED) {
        app_log_info("BLE peer disconnected");
    }
}

static void app_on_transport_rx(const unsigned char *buf, unsigned len, void *user) {
    app_ctx_t *ctx = (app_ctx_t *)user;
    proto_frame_t frame;
    session_action_t action;
    guint64 now_ms;

    if (ctx == NULL || buf == NULL || len == 0U) {
        return;
    }

    if (proto_frame_decode(buf, len, &frame) != 0) {
        app_log_error("protocol decode failed");
        return;
    }

    if (frame.control.fun == 0U) {
        proto_session_reset(&ctx->session);
        (void)app_send_ack(ctx, &frame, 0);
        return;
    }

    if (frame.control.fun == 3U) {
        (void)app_send_ack(ctx, &frame, 0);
        return;
    }

    now_ms = (guint64)(g_get_monotonic_time() / 1000);
    action = proto_session_accept_frame(&ctx->session, &frame, now_ms);
    if (action == SESSION_ACTION_NACK) {
        (void)app_send_ack(ctx, &frame, 1);
        return;
    }

    (void)app_send_ack(ctx, &frame, 0);

    if (action == SESSION_ACTION_COMPLETE) {
        unsigned char packet[10240];
        unsigned packet_len = proto_session_build_packet(&ctx->session, packet, sizeof(packet));
        unsigned char reply_payload[160];
        unsigned reply_len = sizeof(reply_payload);

        if (packet_len == 0U) {
            app_log_error("packet rebuild failed");
            return;
        }

        if (proto_dispatch_packet(packet, packet_len, frame.prot, reply_payload, &reply_len) != 0) {
            app_log_warn("protocol dispatch rejected prot=0x%02X", frame.prot);
            return;
        }

        if (app_send_business_reply(ctx, &frame, reply_payload, reply_len) != 0) {
            app_log_error("send business reply failed");
        }
    }
}

static gboolean app_poll_transport(gpointer user_data) {
    app_ctx_t *ctx = (app_ctx_t *)user_data;

    if (ctx == NULL || ctx->transport == NULL) {
        return G_SOURCE_CONTINUE;
    }

    if (transport_poll(ctx->transport, 0) != 0) {
        app_log_warn("transport poll returned error");
    }
    return G_SOURCE_CONTINUE;
}

int main(int argc, char **argv) {
    GMainLoop *loop = NULL;
    app_config_t cfg;
    app_ctx_t app;
    transport_open_args_t args;
    const char *cfg_path = "conf/sensor_terminal.ini";

    memset(&app, 0, sizeof(app));
    memset(&args, 0, sizeof(args));

    if (argc > 1) {
        cfg_path = argv[1];
    }

    app_config_set_defaults(&cfg);
    if (app_config_load(cfg_path, &cfg) != 0) {
        app_log_warn("failed to load config: %s, using defaults", cfg_path);
    }

    transport_socket_set_backend(transport_ble_gatt_backend());

    args.device_name = cfg.device_name;
    args.service_uuid = cfg.service_uuid;
    args.rx_uuid = cfg.rx_uuid;
    args.tx_uuid = cfg.tx_uuid;
    args.rx_cb = app_on_transport_rx;
    args.event_cb = app_on_transport_event;
    args.user = &app;

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

    app.loop = loop;
    proto_session_reset(&app.session);
    if (transport_open(&app.transport, &args) != 0) {
        app_log_error("transport_open failed");
        g_main_loop_unref(loop);
        return 1;
    }

    g_timeout_add(50U, app_poll_transport, &app);
    app_log_info("sensor terminal skeleton started");
    g_main_loop_run(loop);

    if (app.transport != NULL) {
        (void)transport_close(app.transport);
    }
    g_main_loop_unref(loop);
    return 0;
}
