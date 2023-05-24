/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string>
#include "vacuum-meter-bt.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#ifdef WIFI
#include "pico/cyw43_arch.h"
#endif

repeating_timer_t timer;
uint16_t g_delta = 0;
uint16_t g_pressure_atmo = 0;
volatile uint16_t g_vacuum_1 = 0;
volatile uint16_t g_vacuum_2 = 0;

int setup() {
    stdio_init_all();

    adc_init();
    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(26);
    adc_gpio_init(27);
#ifdef WIFI
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed");
        return -1;
    }
#endif
    return 0;
}

void loop() {
#ifdef WIFI
    static bool out = true;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, out);
    out = !out;
#endif
}

int main() {
    if (setup() != 0) {
        return 1;
    }
    while (true) {
        loop();
    }

    cancel_repeating_timer(&timer);
    return 0;
}

std::vector<int> synchronization() {
    static int pressure_V1;
    static int pressure_V2;

    pressure_V1 =
        map(g_vacuum_1, VACCUM_AMIN, VACCUM_AMAX, VACCUM_VMIN, VACCUM_VMAX);
    pressure_V2 =
        map(g_vacuum_2, VACCUM_AMIN, VACCUM_AMAX, VACCUM_VMIN, VACCUM_VMAX);
    pressure_V2 = pressure_V2 + g_delta;

    std::vector<int> vec{pressure_V1, pressure_V2};
    return vec;
}

std::vector<int> pressure_diff() {
    static int pressure_V1;

    pressure_V1 =
        map(g_vacuum_1, VACCUM_AMIN, VACCUM_AMAX, VACCUM_VMIN, VACCUM_VMAX) -
        g_pressure_atmo;
    std::vector<int> vec{pressure_V1};
    return vec;
}

std::vector<int> pressure_absolute() {
    static int pressure_V1;

    pressure_V1 =
        map(g_vacuum_1, VACCUM_AMIN, VACCUM_AMAX, VACCUM_VMIN, VACCUM_VMAX);
    std::vector<int> vec{pressure_V1};
    return vec;
}

bool calibrate() {
    static unsigned int v1 = 0, v2 = 0;
    static uint8_t cnt = 0;
    v1 += g_vacuum_1;
    v2 += g_vacuum_2;

    if (++cnt == 10) {
        g_pressure_atmo =
            map((v1 / 10), VACCUM_AMIN, VACCUM_AMAX, VACCUM_VMIN, VACCUM_VMAX);
        g_delta = g_pressure_atmo - map((v2 / 10), VACCUM_AMIN, VACCUM_AMAX,
                                        VACCUM_VMIN, VACCUM_VMAX);
        cnt = 0;
        return true;
    }
    return false;
}

bool timer_callback(repeating_timer_t *rt) {
    unsigned int sum_a0 = 0, sum_a1 = 0;
    // encoder.update();
    for (uint8_t i = 0; i < ADC_SAMPLES; ++i) {
        // Select ADC input 0 (GPIO26)
        adc_select_input(0);
        sum_a0 += (adc_read() * 3300 / 4096);
        // Select ADC input 0 (GPIO26)
        adc_select_input(1);
        sum_a1 += (adc_read() * 3300 / 4096);
    }
    g_vacuum_1 = constrain(sum_a0 / ADC_SAMPLES, VACCUM_AMIN, VACCUM_AMAX);
    g_vacuum_2 = constrain(sum_a1 / ADC_SAMPLES, VACCUM_AMIN, VACCUM_AMAX);
    return true; // keep repeating
}
