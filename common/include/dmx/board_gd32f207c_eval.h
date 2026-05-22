/**
 * @file board_gd32f207c_eval.h
 *
 */
/* Copyright (C) 2021-2026 by Arjan van Vught mailto:info@gd32-dmx.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef DMX_BOARD_GD32F207C_EVAL_H_
#define DMX_BOARD_GD32F207C_EVAL_H_

#include <cstdint>

#include "gd32.h" // IWYU pragma: keep

#define DMX_MAX_PORTS 2

namespace dmx::config {
namespace max {
inline constexpr uint32_t kPorts = DMX_MAX_PORTS;
} // namespace max

// #define DMX_USE_USART0
#define DMX_USE_USART1
#define DMX_USE_USART2
// #define DMX_USE_UART3
// #define DMX_USE_UART4
// #define DMX_USE_USART5
// #define DMX_USE_UART6
// #define DMX_USE_UART7

// inline constexpr auto kUsart0Port = 0;
inline constexpr auto kUsart1Port = 0;
inline constexpr auto kUsart2Port = 1;
// inline constexpr auto kUart3Port = 2;
// inline constexpr auto kUart4Port = 3;
// inline constexpr auto kUsart5Port = 5;
// inline constexpr auto kUart6Port = 6;
// inline constexpr auto kUart7Port = 7;

inline constexpr auto kDirPort0GpioPort = GPIOE;
inline constexpr auto kDirPort0GpioPin = GPIO_PIN_9;

inline constexpr auto kDirPort1GpioPort = GPIOE;
inline constexpr auto kDirPort1GpioPin = GPIO_PIN_10;

inline constexpr auto kDirPort2GpioPort = GPIOE;
inline constexpr auto kDirPort2GpioPin = GPIO_PIN_11;

inline constexpr auto kDirPort3GpioPort = GPIOE;
inline constexpr auto kDirPort3GpioPin = GPIO_PIN_12;

inline constexpr auto kDirPort4GpioPort = GPIOE;
inline constexpr auto kDirPort4GpioPin = GPIO_PIN_13;

inline constexpr auto kDirPort5GpioPort = GPIOE;
inline constexpr auto kDirPort5GpioPin = GPIO_PIN_14;

inline constexpr auto kDirPort6GpioPort = GPIOE;
inline constexpr auto kDirPort6GpioPin = GPIO_PIN_15;

inline constexpr auto kDirPort7GpioPort = GPIOB;
inline constexpr auto kDirPort7GpioPin = GPIO_PIN_15;
} // namespace dmx::config
#endif // DMX_BOARD_GD32F207C_EVAL_H_
