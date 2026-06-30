#include "proto_session.h"

#include <string.h>

void proto_session_reset(proto_session_t *session) {
    if (session == NULL) {
        return;
    }
    memset(session, 0, sizeof(*session));
}
