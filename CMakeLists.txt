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

target_include_directories(ble_sensor_terminal PRIVATE
    include
    src
    ${GLIB2_INCLUDE_DIRS}
)

target_compile_options(ble_sensor_terminal PRIVATE
    -Wall -Wextra -Werror -g
)

target_link_libraries(ble_sensor_terminal PRIVATE
    ${GLIB2_LIBRARIES}
    pthread
)
