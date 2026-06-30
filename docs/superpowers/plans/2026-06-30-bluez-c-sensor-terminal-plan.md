# BlueZ C Sensor Terminal Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build `project003_ble_sensor_terminal` as a standalone Linux BLE peripheral and GATT server in C, acting as the protocol-defined sensor terminal, able to receive requests from a Linux central and send protocol replies back.

**Architecture:** The project stays independent from `project001_bluetooth` and `project002_ble_host_demo`. BlueZ D-Bus is used to expose one custom GATT service with RX write and TX notify characteristics, while the application protocol runs above a dedicated transport abstraction layer that exposes socket-style APIs so the protocol/session code does not depend directly on BlueZ.

**Tech Stack:** C11, CMake, BlueZ D-Bus GATT API, GLib/GIO, pthread, btmon, bluetoothctl, optional Python smoke tools

---

## File Structure

Project root:

- `D:\Coding\projects\project003_ble_sensor_terminal\`

Planned source tree:

- Create: `D:\Coding\projects\project003_ble_sensor_terminal\CMakeLists.txt`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\README.md`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\conf\sensor_terminal.ini`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\include\app_config.h`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\include\app_log.h`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\include\proto_frame.h`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\include\proto_session.h`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\include\proto_dispatch.h`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\include\transport_api.h`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\include\transport_socket.h`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\include\transport_ble_gatt.h`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\main.c`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\app_config.c`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\app_log.c`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\proto_frame.c`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\proto_session.c`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\proto_dispatch.c`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\transport_api.c`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\transport_socket.c`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\transport_ble_gatt.c`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\ble_dbus_app.c`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\ble_dbus_app.h`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\tx_queue.c`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\tx_queue.h`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\tests\test_proto_frame.c`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\tests\test_proto_session.c`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\tools\central_smoke.py`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\tools\send_frame.py`

### Architecture Boundaries

Split the implementation into four stable layers:

1. `transport_api`
   - Provides socket-style transport boundary.
   - Defines `open/close/send/recv/poll/set_callback`.
   - Upper protocol code only depends on this layer.
2. `transport_ble_gatt`
   - Implements the `transport_api` boundary on top of BlueZ GATT RX/TX.
   - Converts RX characteristic writes into transport receive events.
   - Converts transport sends into TX notifications.
3. `proto_frame + proto_session + proto_dispatch`
   - Owns framing, checksum, PSEQ/FSEQ, reassembly, repair request, and business reply generation.
4. `ble_dbus_app`
   - Owns BlueZ object registration, advertising, and D-Bus event loop.

### Socket-Style Transport Interface

The transport boundary must be defined first and kept stable:

```c
typedef struct transport_handle transport_handle_t;

typedef void (*transport_rx_cb)(const unsigned char *buf, unsigned len, void *user);
typedef void (*transport_event_cb)(int event, void *user);

typedef struct {
    const char *device_name;
    const char *service_uuid;
    const char *rx_uuid;
    const char *tx_uuid;
    transport_rx_cb rx_cb;
    transport_event_cb event_cb;
    void *user;
} transport_open_args_t;

int transport_open(transport_handle_t **handle, const transport_open_args_t *args);
int transport_close(transport_handle_t *handle);
int transport_send(transport_handle_t *handle, const unsigned char *buf, unsigned len);
int transport_poll(transport_handle_t *handle, int timeout_ms);
```

The first implementation lives in `transport_ble_gatt.c`, but the API is intentionally socket-shaped so a future Unix domain socket, TCP mock transport, or platform BLE module can share the same upper-layer protocol code.

## Task 1: Create project003 skeleton

**Files:**
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\CMakeLists.txt`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\README.md`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\main.c`

- [ ] **Step 1: Create the basic directories**

Create:

```text
D:\Coding\projects\project003_ble_sensor_terminal\
  conf\
  include\
  src\
  tests\
  tools\
  docs\superpowers\plans\
```

- [ ] **Step 2: Add the standalone CMake target**

`D:\Coding\projects\project003_ble_sensor_terminal\CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.16)
project(ble_sensor_terminal C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 REQUIRED glib-2.0 gio-2.0)

add_executable(ble_sensor_terminal
    src/main.c
    src/app_config.c
    src/app_log.c
    src/proto_frame.c
    src/proto_session.c
    src/proto_dispatch.c
    src/transport_api.c
    src/transport_socket.c
    src/transport_ble_gatt.c
    src/ble_dbus_app.c
    src/tx_queue.c
)

target_include_directories(ble_sensor_terminal PRIVATE include ${GLIB2_INCLUDE_DIRS})
target_compile_options(ble_sensor_terminal PRIVATE -Wall -Wextra -Werror -g)
target_link_libraries(ble_sensor_terminal PRIVATE ${GLIB2_LIBRARIES} pthread)
```

- [ ] **Step 3: Add the main loop skeleton**

`D:\Coding\projects\project003_ble_sensor_terminal\src\main.c`

```c
#include <glib.h>

int main(void) {
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
    g_main_loop_unref(loop);
    return 0;
}
```

- [ ] **Step 4: Add the initial README**

Document:
- project purpose
- central/peripheral roles
- RX write + TX notify model
- transport layer abstraction rule

- [ ] **Step 5: Configure and build the empty target**

Run:

```powershell
cmake -S D:\Coding\projects\project003_ble_sensor_terminal -B D:\Coding\projects\project003_ble_sensor_terminal\build
cmake --build D:\Coding\projects\project003_ble_sensor_terminal\build -j
```

Expected:
- configure succeeds
- `ble_sensor_terminal` binary is generated

- [ ] **Step 6: Commit**

```bash
git add D:/Coding/projects/project003_ble_sensor_terminal
git commit -m "build: scaffold project003 BLE sensor terminal"
```

## Task 2: Add config and logging

**Files:**
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\include\app_config.h`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\include\app_log.h`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\app_config.c`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\app_log.c`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\conf\sensor_terminal.ini`

- [ ] **Step 1: Define runtime config**

`D:\Coding\projects\project003_ble_sensor_terminal\include\app_config.h`

```c
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
```

- [ ] **Step 2: Add default config**

`D:\Coding\projects\project003_ble_sensor_terminal\conf\sensor_terminal.ini`

```ini
device_name=sensor-terminal-demo
service_uuid=9b0a0001-5f3b-4b5c-8c71-000000000001
rx_uuid=9b0a0002-5f3b-4b5c-8c71-000000000001
tx_uuid=9b0a0003-5f3b-4b5c-8c71-000000000001
frame_timeout_ms=3000
resend_limit=2
tx_power_dbm=5
enable_repair=1
```

- [ ] **Step 3: Add logging helpers**

`D:\Coding\projects\project003_ble_sensor_terminal\include\app_log.h`

```c
void app_log_info(const char *fmt, ...);
void app_log_warn(const char *fmt, ...);
void app_log_error(const char *fmt, ...);
void app_log_hex(const char *tag, const unsigned char *buf, unsigned len);
```

- [ ] **Step 4: Print config at startup**

Expected log:

```text
INFO device=sensor-terminal-demo timeout=3000 resend=2 repair=1
```

- [ ] **Step 5: Rebuild**

Run:

```powershell
cmake --build D:\Coding\projects\project003_ble_sensor_terminal\build -j
```

Expected:
- build succeeds

- [ ] **Step 6: Commit**

```bash
git add D:/Coding/projects/project003_ble_sensor_terminal/include/app_config.h D:/Coding/projects/project003_ble_sensor_terminal/include/app_log.h D:/Coding/projects/project003_ble_sensor_terminal/src/app_config.c D:/Coding/projects/project003_ble_sensor_terminal/src/app_log.c D:/Coding/projects/project003_ble_sensor_terminal/conf/sensor_terminal.ini
git commit -m "feat: add config and logging for project003"
```

## Task 3: Implement transport abstraction and socket wrapper

**Files:**
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\include\transport_api.h`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\include\transport_socket.h`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\transport_api.c`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\transport_socket.c`

- [ ] **Step 1: Define the transport ABI**

`D:\Coding\projects\project003_ble_sensor_terminal\include\transport_api.h`

```c
typedef struct transport_handle transport_handle_t;

typedef enum {
    TRANSPORT_EVENT_CONNECTED = 1,
    TRANSPORT_EVENT_DISCONNECTED = 2,
    TRANSPORT_EVENT_NOTIFY_ENABLED = 3,
    TRANSPORT_EVENT_NOTIFY_DISABLED = 4
} transport_event_t;

typedef void (*transport_rx_cb)(const unsigned char *buf, unsigned len, void *user);
typedef void (*transport_event_cb)(int event, void *user);

typedef struct {
    const char *device_name;
    const char *service_uuid;
    const char *rx_uuid;
    const char *tx_uuid;
    transport_rx_cb rx_cb;
    transport_event_cb event_cb;
    void *user;
} transport_open_args_t;

int transport_open(transport_handle_t **handle, const transport_open_args_t *args);
int transport_close(transport_handle_t *handle);
int transport_send(transport_handle_t *handle, const unsigned char *buf, unsigned len);
int transport_poll(transport_handle_t *handle, int timeout_ms);
```

- [ ] **Step 2: Define the socket-style internal wrapper**

`D:\Coding\projects\project003_ble_sensor_terminal\include\transport_socket.h`

```c
typedef struct {
    int (*open)(transport_handle_t **handle, const transport_open_args_t *args);
    int (*close)(transport_handle_t *handle);
    int (*send)(transport_handle_t *handle, const unsigned char *buf, unsigned len);
    int (*poll)(transport_handle_t *handle, int timeout_ms);
} transport_vtable_t;
```

- [ ] **Step 3: Implement `transport_api.c` as the stable entry**

Responsibilities:
- own the active vtable
- forward `transport_open/send/poll/close`
- keep upper layers unaware of BlueZ internals

- [ ] **Step 4: Implement `transport_socket.c` as the adapter boundary**

Responsibilities:
- store the selected backend
- expose one place for future Unix socket or TCP mock backends
- keep function names socket-like

- [ ] **Step 5: Add a transport smoke stub**

Behavior:
- opening the transport without backend returns explicit error
- this is the first failing testable milestone before BLE is wired in

- [ ] **Step 6: Commit**

```bash
git add D:/Coding/projects/project003_ble_sensor_terminal/include/transport_api.h D:/Coding/projects/project003_ble_sensor_terminal/include/transport_socket.h D:/Coding/projects/project003_ble_sensor_terminal/src/transport_api.c D:/Coding/projects/project003_ble_sensor_terminal/src/transport_socket.c
git commit -m "feat: add socket-style transport abstraction"
```

## Task 4: Implement frame codec

**Files:**
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\include\proto_frame.h`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\proto_frame.c`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\tests\test_proto_frame.c`

- [ ] **Step 1: Define frame structure and control fields**

`D:\Coding\projects\project003_ble_sensor_terminal\include\proto_frame.h`

```c
typedef struct {
    unsigned char dir;
    unsigned char prm;
    unsigned char fcb;
    unsigned char fcv;
    unsigned char fun;
} proto_control_t;

typedef struct {
    unsigned short length;
    proto_control_t control;
    unsigned char pseq;
    unsigned char fseq;
    unsigned char prot;
    unsigned char data[160];
    unsigned short data_len;
} proto_frame_t;

int proto_frame_decode(const unsigned char *buf, unsigned len, proto_frame_t *out);
int proto_frame_encode(const proto_frame_t *frame, unsigned char *buf, unsigned *len_io);
unsigned char proto_frame_calc_cs(const unsigned char *buf, unsigned len);
```

- [ ] **Step 2: Write the failing decode test**

`D:\Coding\projects\project003_ble_sensor_terminal\tests\test_proto_frame.c`

```c
static void test_decode_valid_single_frame(void) {
    unsigned char raw[] = {0xA5, 0x05, 0x00, 0x51, 0x01, 0x80, 0x10, 0x00, 0xE2, 0x96};
    proto_frame_t frame = {0};
    g_assert_cmpint(proto_frame_decode(raw, sizeof(raw), &frame), ==, 0);
}
```

- [ ] **Step 3: Run the test to confirm it fails first**

Run:

```powershell
cmake --build D:\Coding\projects\project003_ble_sensor_terminal\build -j
ctest --test-dir D:\Coding\projects\project003_ble_sensor_terminal\build --output-on-failure
```

Expected:
- codec test fails before implementation

- [ ] **Step 4: Implement decode and encode**

Must validate:
- start byte `0xA5`
- end byte `0x96`
- `L` consistency
- checksum correctness
- `DATA <= 160`

- [ ] **Step 5: Add negative tests**

Add:
- bad checksum
- bad length
- bad tail byte

- [ ] **Step 6: Commit**

```bash
git add D:/Coding/projects/project003_ble_sensor_terminal/include/proto_frame.h D:/Coding/projects/project003_ble_sensor_terminal/src/proto_frame.c D:/Coding/projects/project003_ble_sensor_terminal/tests/test_proto_frame.c
git commit -m "feat: implement protocol frame codec"
```

## Task 5: Implement reassembly session

**Files:**
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\include\proto_session.h`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\proto_session.c`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\tests\test_proto_session.c`

- [ ] **Step 1: Define session state**

`D:\Coding\projects\project003_ble_sensor_terminal\include\proto_session.h`

```c
#define PROTO_MAX_FRAMES 64

typedef struct {
    unsigned char active;
    unsigned char current_pseq;
    unsigned char total_frames;
    unsigned char received[PROTO_MAX_FRAMES];
    unsigned short frame_len[PROTO_MAX_FRAMES];
    unsigned char frame_data[PROTO_MAX_FRAMES][160];
    guint64 last_update_ms;
} proto_session_t;
```

- [ ] **Step 2: Define session actions**

```c
typedef enum {
    SESSION_ACTION_ACK = 1,
    SESSION_ACTION_NACK = 2,
    SESSION_ACTION_WAIT = 3,
    SESSION_ACTION_COMPLETE = 4
} session_action_t;
```

- [ ] **Step 3: Implement first-frame and follow-frame handling**

Rules:
- `FIR=1` initializes a packet
- `NUM+1` is the total frame count
- duplicate `PSEQ + FSEQ` returns `ACK`
- invalid sequence returns `NACK`

- [ ] **Step 4: Implement timeout reset**

Rule:
- a partial packet older than `frame_timeout_ms` is cleared when the next frame arrives

- [ ] **Step 5: Write and run session tests**

Cover:
- one-frame complete
- three-frame complete
- duplicate frame
- out-of-range `SEQ`
- timeout reset

- [ ] **Step 6: Commit**

```bash
git add D:/Coding/projects/project003_ble_sensor_terminal/include/proto_session.h D:/Coding/projects/project003_ble_sensor_terminal/src/proto_session.c D:/Coding/projects/project003_ble_sensor_terminal/tests/test_proto_session.c
git commit -m "feat: implement protocol session management"
```

## Task 6: Implement BlueZ D-Bus app and BLE transport backend

**Files:**
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\include\transport_ble_gatt.h`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\transport_ble_gatt.c`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\ble_dbus_app.h`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\ble_dbus_app.c`
- Modify: `D:\Coding\projects\project003_ble_sensor_terminal\src\main.c`

- [ ] **Step 1: Define the BLE backend-private context**

The backend must track:
- `GDBusConnection *`
- adapter path
- application path
- RX/TX characteristic paths
- notify-enabled flag
- outbound queue
- callbacks from `transport_open_args_t`

- [ ] **Step 2: Implement BlueZ adapter bring-up**

Requirements:
- resolve `/org/bluez/hci0`
- set `Powered=true`
- register the custom GATT application
- enter advertising state

- [ ] **Step 3: Implement RX write callback**

Behavior:
- ATT write success means only “transport input accepted”
- copy bytes and invoke `rx_cb`
- do not perform protocol logic in BlueZ object glue

- [ ] **Step 4: Implement TX notify send path**

Behavior:
- `transport_send()` enqueues bytes
- BLE backend turns them into TX characteristic notifications
- if notifications are disabled, return explicit send failure

- [ ] **Step 5: Wire backend into `transport_open()`**

Expected flow:

```c
transport_open(...)
  -> transport_socket_select_backend(...)
  -> transport_ble_gatt_open(...)
  -> BlueZ register app
```

- [ ] **Step 6: Manual verify GATT visibility**

Run on Linux target:

```bash
bluetoothctl show
busctl tree org.bluez
```

Expected:
- adapter powered
- service and characteristics present

- [ ] **Step 7: Commit**

```bash
git add D:/Coding/projects/project003_ble_sensor_terminal/include/transport_ble_gatt.h D:/Coding/projects/project003_ble_sensor_terminal/src/transport_ble_gatt.c D:/Coding/projects/project003_ble_sensor_terminal/src/ble_dbus_app.h D:/Coding/projects/project003_ble_sensor_terminal/src/ble_dbus_app.c D:/Coding/projects/project003_ble_sensor_terminal/src/main.c
git commit -m "feat: add BlueZ D-Bus transport backend"
```

## Task 7: Implement protocol dispatch and reply generation

**Files:**
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\include\proto_dispatch.h`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\src\proto_dispatch.c`
- Modify: `D:\Coding\projects\project003_ble_sensor_terminal\src\main.c`

- [ ] **Step 1: Define dispatch contract**

`D:\Coding\projects\project003_ble_sensor_terminal\include\proto_dispatch.h`

```c
int proto_dispatch_packet(
    const unsigned char *packet,
    unsigned packet_len,
    unsigned char prot,
    unsigned char *reply_buf,
    unsigned *reply_len_io
);
```

- [ ] **Step 2: Implement first-stage mock business logic**

Rules:
- `PROT=0x10` returns a mock sensor-terminal payload
- `PROT=0x00` and `0x01` return a supported-but-not-implemented payload
- unknown `PROT` returns protocol-level `NACK`

- [ ] **Step 3: Build the receive pipeline in `main.c`**

Flow:

```c
transport rx callback
  -> proto_frame_decode
  -> proto_session_accept
  -> send ACK or NACK
  -> if packet complete:
       proto_dispatch_packet
       proto_frame_encode(response)
       transport_send(response)
```

- [ ] **Step 4: Add `FUN=0` and `FUN=3` handlers**

Rules:
- reset clears session and replies `ACK`
- link test replies `ACK`

- [ ] **Step 5: Manual verify single-frame loop**

Expected:
- central writes a valid frame
- target replies one `ACK`
- target replies one business response

- [ ] **Step 6: Commit**

```bash
git add D:/Coding/projects/project003_ble_sensor_terminal/include/proto_dispatch.h D:/Coding/projects/project003_ble_sensor_terminal/src/proto_dispatch.c D:/Coding/projects/project003_ble_sensor_terminal/src/main.c
git commit -m "feat: add dispatch and protocol reply pipeline"
```

## Task 8: Implement repair-request and duplicate handling

**Files:**
- Modify: `D:\Coding\projects\project003_ble_sensor_terminal\src\proto_session.c`
- Modify: `D:\Coding\projects\project003_ble_sensor_terminal\src\proto_dispatch.c`
- Modify: `D:\Coding\projects\project003_ble_sensor_terminal\README.md`

- [ ] **Step 1: Add repair-request handling**

Rules:
- `FUN=2` inspects current session state
- determine missing frames
- generate repair-confirm or repair-deny response frame

- [ ] **Step 2: Add duplicate-frame protection**

Rules:
- duplicate `PSEQ + FSEQ` is acknowledged again
- business logic is not re-executed for a fully processed duplicate

- [ ] **Step 3: Document error flows**

README must cover:
- checksum error
- sequence error
- notify disabled
- timeout reset
- repair request

- [ ] **Step 4: Commit**

```bash
git add D:/Coding/projects/project003_ble_sensor_terminal/src/proto_session.c D:/Coding/projects/project003_ble_sensor_terminal/src/proto_dispatch.c D:/Coding/projects/project003_ble_sensor_terminal/README.md
git commit -m "feat: implement repair and duplicate handling"
```

## Task 9: Add central-side smoke tools and verification guide

**Files:**
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\tools\central_smoke.py`
- Create: `D:\Coding\projects\project003_ble_sensor_terminal\tools\send_frame.py`
- Modify: `D:\Coding\projects\project003_ble_sensor_terminal\README.md`

- [ ] **Step 1: Add the central smoke tool**

Responsibilities:
- scan by device name
- connect
- subscribe TX notify
- write one raw request frame to RX
- print reply frames as hex

- [ ] **Step 2: Add a request-frame builder tool**

Responsibilities:
- input `FUN`, `PROT`, and payload
- assemble a frame
- write to RX

- [ ] **Step 3: Verify positive cases**

Check:
- one-frame request -> `ACK + response`
- multi-frame request -> per-frame `ACK` + final response
- reset -> `ACK`
- link test -> `ACK`

- [ ] **Step 4: Verify negative cases**

Check:
- checksum error -> `NACK`
- bad `SEQ` -> `NACK`
- duplicate frame -> repeated `ACK` without duplicate business response
- notify disabled -> write accepted but response path logs explicit failure

- [ ] **Step 5: Capture btmon trace**

Run:

```bash
sudo btmon
```

Expected:
- ATT write requests appear on RX
- handle value notifications appear on TX

- [ ] **Step 6: Commit**

```bash
git add D:/Coding/projects/project003_ble_sensor_terminal/tools/central_smoke.py D:/Coding/projects/project003_ble_sensor_terminal/tools/send_frame.py D:/Coding/projects/project003_ble_sensor_terminal/README.md
git commit -m "test: add smoke tools and verification guide"
```

## Milestones

### Milestone 1: Project shell
- project003 directory is created
- `PROJECT_SYNC.md` exists
- standalone build target configures

### Milestone 2: Stable transport boundary
- protocol code compiles against `transport_api.h`
- no protocol file includes BlueZ-specific headers

### Milestone 3: Protocol core
- codec and session tests pass
- single-frame encode/decode works

### Milestone 4: BLE transport loop
- BlueZ app registers
- central can write RX and subscribe TX

### Milestone 5: Full request-response
- valid request yields `ACK`
- valid request yields business reply
- repair and duplicate flows are stable

## Verification Checklist

- `project003_ble_sensor_terminal` exists under `projects/`
- `PROJECT_SYNC.md` exists at the project root
- build works independently of `project001` and `project002`
- transport abstraction is socket-style and isolated from protocol code
- BlueZ-specific code is confined to `transport_ble_gatt.c` and `ble_dbus_app.c`
- RX write and TX notify semantics are separated
- protocol frame split rules do not depend on ATT MTU
- checksum, sequence, timeout, reset, link-test, and repair flows are covered

## Recommended execution order

Implement Tasks 1 through 3 first, then stop and verify the transport abstraction shape. If that boundary is wrong, the later BlueZ work will be expensive to unwind. After that, do Tasks 4 through 7 to reach the first closed request-response loop, and finish with Tasks 8 through 9 for hardening and smoke verification.
