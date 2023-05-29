/// @copyright Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
/// SPDX-License-Identifier: BSD-3-Clause
#pragma once

/// @brief Initialise BTstack with cyw43
/// @return 0 if ok
int bluetooth_init(void);

/// @brief Run the BTstack setup
void bt_stack_setup(void);
