/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "btstack_run_loop.h"
#include "pico/stdlib.h"
#include "common.h"

int main() {
    stdio_init_all();

    int res = bluetooth_init();
    if (res){
        return -1;
    }

    bt_stack_setup();
    btstack_run_loop_execute();
}
