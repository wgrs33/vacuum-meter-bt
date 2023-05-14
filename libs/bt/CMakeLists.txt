#< Adds common stuff for all the examples
add_executable(spp_streamer
        ${CMAKE_CURRENT_LIST_DIR}/common.c
        ${CMAKE_CURRENT_LIST_DIR}/background.c
        ${CMAKE_CURRENT_LIST_DIR}/spp_streamer.c
        )

target_link_libraries(spp_streamer PRIVATE
        pico_stdlib
        pico_btstack_ble
        pico_btstack_classic
        pico_btstack_cyw43
        pico_cyw43_arch_threadsafe_background
        )

target_include_directories(spp_streamer PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/config # Use our own config
        )

target_compile_definitions(spp_streamer PRIVATE
        PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000
        CYW43_LWIP=0
        )

suppress_btstack_warnings()

pico_add_extra_outputs(spp_streamer)
