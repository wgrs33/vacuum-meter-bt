# generate header from GATT file
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/gatt_streamer_server.h
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.gatt
  COMMAND ${PICO_SDK_PATH}/lib/btstack/tool/compile_gatt.py -I ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/gatt_streamer_server.gatt
      ${CMAKE_CURRENT_BINARY_DIR}/gatt_streamer_server.h
  VERBATIM
)

#<< Adds common stuff for all the examples
add_executable(le_streamer
        ${CMAKE_CURRENT_SOURCE_DIR}/common.c
        ${CMAKE_CURRENT_SOURCE_DIR}/background.c
        ${CMAKE_CURRENT_SOURCE_DIR}/gatt_streamer_server.c
        ${CMAKE_CURRENT_BINARY_DIR}/gatt_streamer_server.h
        )


target_link_libraries(le_streamer PRIVATE
        pico_stdlib
        pico_btstack_ble
        pico_btstack_cyw43
        pico_cyw43_arch_threadsafe_background
        )

target_include_directories(le_streamer PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/config # Use our own config
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        )

target_compile_definitions(le_streamer PRIVATE
        PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000
        CYW43_LWIP=0
        )

suppress_btstack_warnings()

pico_add_extra_outputs(le_streamer)
