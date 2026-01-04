/**
 * @file pixeloutputmulti_config.h
 *
 */
/* Copyright (C) 2021-2025 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef GD32_GPIO_PIXELOUTPUTMULTI_CONFIG_H_
#define GD32_GPIO_PIXELOUTPUTMULTI_CONFIG_H_

#if defined(BOARD_GD32F207RG)
#include "board_gd32f207rg.h"
#elif defined(BOARD_GD32F407RE)
#include "board_gd32f407re.h"
#elif defined(BOARD_GD32F450VI)
#include "board_gd32f450vi.h"
#elif defined(BOARD_16X4U_PIXEL)
#include "board_gd32f450vi.h"
#elif defined(BOARD_GD32F470VG)
#include "board_gd32f470vg.h"
#elif defined(BOARD_GD32F207C_EVAL)
#include "board_gd32f207c_eval.h"
#elif defined(BOARD_GD32F470Z_EVAL)
#include "board_gd32f470z_eval.h"
#else
#error Board is not supported
#endif

namespace pixel
{
static constexpr auto kPortCount = __builtin_popcount(GPIO_PINx);
static_assert(kPortCount <= 16, "Too many ports");
} // namespace pixel

#endif // GD32_GPIO_PIXELOUTPUTMULTI_CONFIG_H_
