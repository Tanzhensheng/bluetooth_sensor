# ble_sensor_terminal

Linux-side BLE sensor terminal demo project for the protocol described in the
Bluetooth transport PDF.

## Scope

- BLE role: Peripheral + GATT Server
- Peer role: Linux Central
- GATT model: RX write + TX notify
- Protocol model: application-layer frame transport above BLE

## Architecture

- `transport_api`: stable socket-style transport boundary for upper layers
- `transport_socket`: backend selector and socket-like wrapper
- `transport_ble_gatt`: BlueZ-backed transport implementation
- `proto_*`: protocol frame, session, and dispatch layers

Upper protocol code must depend only on `transport_open`,
`transport_send`, `transport_poll`, and `transport_close`.

## Build

This project is intended to build on Linux with:

- `cmake`
- `pkg-config`
- `glib-2.0`
- `gio-2.0`

Example:

```bash
cmake -S . -B build
cmake --build build -j
```
