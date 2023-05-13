#pragma once

#include <cstdint>
#include <math.h>
#include "pico/util/queue.h"

constexpr int BUTTON = 8;
constexpr int VACCUM_1 = 1;//A1;
constexpr int VACCUM_2 = 0;//A0;

constexpr int BAR_MIN = 0;
constexpr int BAR_MAX = 70;

constexpr unsigned int VACCUM_AMIN = 132U; // 132mV
constexpr unsigned int VACCUM_AMAX = 3168U; // 3168mV
constexpr unsigned int VACCUM_VMIN = 200U;
constexpr unsigned int VACCUM_VMAX = 2500U;
constexpr int VACCUM_DMIN = -800;
constexpr int VACCUM_DMAX = 1500;
constexpr int FIFO_LENGTH = 32;

constexpr double c_MMHG = 1.33322;
constexpr unsigned int ADC_SAMPLES = 10U;

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/// @brief Show user menu
void show_menu();

/// @brief Show synchronization status (2 strokes)
void synchronization();

/// @brief Show differential pressure
void pressure_diff();

/// @brief Show absolute pressure with mbar and mmHg
void pressure_absolute();

/// @brief Set bar graph value
/// @param value value in the range [0 - 70]
void set_bar(int value);

/// @brief Align text to right
/// @param value Value to show
/// @param max max length
void align_right(int value, int max);

/// @brief Calibrate inputs
/// @details Both inputs must not be connected to any source.
/// Atmospheric pressure is stored.
/// Delta is calculated between two inputs.
/// @return True if calibration has finished
bool calibrate();

/// @brief Timer callback when timer hit OC
/// @param rt Timer handle
/// @return 
bool timer_callback(repeating_timer_t *rt);

void updateLcd();