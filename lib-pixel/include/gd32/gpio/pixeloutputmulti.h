/**
 * @file pixeloutputmulti.h
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

#ifndef GD32_GPIO_PIXELOUTPUTMULTI_H_
#define GD32_GPIO_PIXELOUTPUTMULTI_H_

#if defined(DEBUG_PIXEL)
#if defined(NDEBUG)
#undef NDEBUG
#define _NDEBUG
#endif
#endif

#pragma GCC push_options
#pragma GCC optimize("O3")
#pragma GCC optimize("no-tree-loop-distribute-patterns")

#include <cstdint>

#include "pixeltype.h"
#include "gd32/gpio/pixeloutputmulti_config.h"
#include "gd32.h"

class PixelOutputMulti
{
   public:
    PixelOutputMulti();
    ~PixelOutputMulti() = default;

    void ApplyConfiguration();

#define BIT_SET(Addr, Bit)                                                                                          \
    {                                                                                                               \
        *(volatile uint32_t*)(BITBAND_SRAM_BASE + (((uint32_t)&Addr) - SRAM_BASE) * 32U + (Bit & 0xFF) * 4U) = 0x1; \
    }

#define BIT_CLEAR(Addr, Bit)                                                                                          \
    {                                                                                                                 \
        *(volatile uint32_t*)(BITBAND_SRAM_BASE + (((uint32_t)&(Addr)) - SRAM_BASE) * 32U + (Bit & 0xFF) * 4U) = 0x0; \
    }

    void SetColourRTZ(uint32_t port_index, uint32_t pixel_index, uint8_t colour1, uint8_t colour2, uint8_t colour3)
    {
        assert(port_index < pixel::kPortCount);

        uint32_t j = 0;
        const auto kIndex = pixel_index * pixel::single::RGB;
        const auto kBit = port_index + GPIO_PIN_OFFSET;
        auto* p = &s_pixel_buffer_data[kIndex];

        for (uint8_t mask = 0x80; mask != 0; mask = static_cast<uint8_t>(mask >> 1))
        {
            auto& p1 = p[j];
            auto& p2 = p[8 + j];
            auto& p3 = p[16 + j];

            if (!(mask & colour1))
            {
                BIT_SET(p1, kBit);
            }
            else
            {
                BIT_CLEAR(p1, kBit);
            }
            if (!(mask & colour2))
            {
                BIT_SET(p2, kBit);
            }
            else
            {
                BIT_CLEAR(p2, kBit);
            }
            if (!(mask & colour3))
            {
                BIT_SET(p3, kBit);
            }
            else
            {
                BIT_CLEAR(p3, kBit);
            }

            j++;
        }
    }

    void SetColourRTZ(uint32_t port_index, uint32_t pixel_index, uint8_t red, uint8_t green, uint8_t blue, uint8_t white)
    {
        assert(port_index < pixel::kPortCount);

        const auto kIndex = pixel_index * pixel::single::RGBW;
        const auto kBit = port_index + GPIO_PIN_OFFSET;

        auto* p = &s_pixel_buffer_data[kIndex];
        uint32_t j = 0;

        for (uint8_t mask = 0x80; mask != 0; mask = static_cast<uint8_t>(mask >> 1))
        {
            auto& p1 = p[j];
            auto& p2 = p[8 + j];
            auto& p3 = p[16 + j];
            auto& p4 = p[24 + j];

            // GRBW
            if (!(mask & green))
            {
                BIT_SET(p1, kBit);
            }
            else
            {
                BIT_CLEAR(p1, kBit);
            }

            if (!(mask & red))
            {
                BIT_SET(p2, kBit);
            }
            else
            {
                BIT_CLEAR(p2, kBit);
            }

            if (!(mask & blue))
            {
                BIT_SET(p3, kBit);
            }
            else
            {
                BIT_CLEAR(p3, kBit);
            }

            if (!(mask & white))
            {
                BIT_SET(p4, kBit);
            }
            else
            {
                BIT_CLEAR(p4, kBit);
            }

            j++;
        }
    }

    void SetColourWS2801(uint32_t port_index, uint32_t pixel_index, uint8_t colour1, uint8_t colour2, uint8_t colour3)
    {
        assert(port_index < pixel::kPortCount);

        uint32_t j = 0;
        const auto kIndex = pixel_index * pixel::single::RGB;
        const auto kBit = port_index + GPIO_PIN_OFFSET;
        auto* p = &s_pixel_buffer_data[kIndex];

        for (uint8_t mask = 0x80; mask != 0; mask = static_cast<uint8_t>(mask >> 1))
        {
            auto& p1 = p[j];
            auto& p2 = p[8 + j];
            auto& p3 = p[16 + j];

            if (mask & colour1)
            {
                BIT_SET(p1, kBit);
            }
            else
            {
                BIT_CLEAR(p1, kBit);
            }
            if (mask & colour2)
            {
                BIT_SET(p2, kBit);
            }
            else
            {
                BIT_CLEAR(p2, kBit);
            }
            if (mask & colour3)
            {
                BIT_SET(p3, kBit);
            }
            else
            {
                BIT_CLEAR(p3, kBit);
            }

            j++;
        }
    }

    void SetPixel4Bytes(uint32_t port_index, uint32_t pixel_index, uint8_t ctrl, uint8_t colour1, uint8_t colour2, uint8_t colour3)
    {
        assert(port_index < pixel::kPortCount);

        uint32_t j = 0;
        const auto kIndex = pixel_index * pixel::single::RGBW;
        const auto kBit = port_index + GPIO_PIN_OFFSET;
        auto* p = &s_pixel_buffer_data[kIndex];

        for (uint8_t mask = 0x80; mask != 0; mask = static_cast<uint8_t>(mask >> 1))
        {
            if (mask & ctrl)
            {
                BIT_SET(p[j], kBit);
            }
            else
            {
                BIT_CLEAR(p[j], kBit);
            }

            if (mask & colour1)
            {
                BIT_SET(p[8 + j], kBit);
            }
            else
            {
                BIT_CLEAR(p[8 + j], kBit);
            }

            if (mask & colour2)
            {
                BIT_SET(p[16 + j], kBit);
            }
            else
            {
                BIT_CLEAR(p[16 + j], kBit);
            }

            if (mask & colour3)
            {
                BIT_SET(p[24 + j], kBit);
            }
            else
            {
                BIT_CLEAR(p[24 + j], kBit);
            }

            j++;
        }
    }

    bool IsUpdating();

    void Update();
    void Blackout();
    void FullOn();

    uint32_t GetUserData();

    static PixelOutputMulti* Get() { return s_this; }

   private:
    void Setup(uint8_t low_code, uint8_t high_code);
    void Setup(uint32_t frequency);

   private:
    uint32_t buffer_size_{0};

    /**
     * https://www.gd32-dmx.org/memory.html
     */
#if defined(GD32F20X) || defined(GD32F4XX)
#define SECTION_PIXEL __attribute__((section(".pixel")))
#else
#define SECTION_PIXEL
#endif

    static inline uint16_t s_pixel_buffer[2 * 1024 * 16] __attribute__((aligned(4))) SECTION_PIXEL;
    static inline constexpr auto kPixelBufferSize = sizeof(s_pixel_buffer) / sizeof(s_pixel_buffer[0]);
    static inline auto* s_pixel_buffer_data = reinterpret_cast<uint16_t*>(s_pixel_buffer);
    static inline auto* s_pixel_buffer_dma = reinterpret_cast<uint16_t*>(s_pixel_buffer + kPixelBufferSize / 2);
    static inline constexpr uint32_t kMaxApA102 = ((kPixelBufferSize / 8) - 8) / 4;

    static inline PixelOutputMulti* s_this;
};

#pragma GCC pop_options
#if defined(_NDEBUG)
#undef _NDEBUG
#define NDEBUG
#endif

#endif // GD32_GPIO_PIXELOUTPUTMULTI_H_
