/**
 * @file pixeloutputmulti.cpp
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

#if defined(DEBUG_PIXEL)
#undef NDEBUG
#endif

#include <cstdint>
#include <cassert>

#include "pixeloutputmulti.h"
#include "pixelconfiguration.h"
#include "pixeltype.h"
#include "gd32/gpio/pixeloutputmulti_config.h"
#include "gd32_gpio.h"
#include "gd32_dma.h"
#include "gd32_dma_memcpy32.h"
#include "firmware/debug/debug_debug.h"

namespace pixel
{
static constexpr uint16_t kGpioPins[] __attribute__((aligned(4))) = {GPIO_PINx};
static const auto* const kSPGpioPiNs = reinterpret_cast<const uint32_t*>(&kGpioPins[0]);
static constexpr uint32_t kRtzTimerPeriod = (0.00000125f * MASTER_TIMER_CLOCK) - 1U;
} // namespace pixel

static volatile bool sv_is_running;
static volatile uint32_t sv_updates_per_second;
static volatile uint32_t sv_updates_previous;
static volatile uint32_t sv_updates;

static void Timer10Config()
{
    sv_updates_per_second = 0;
    sv_updates_previous = 0;
    sv_updates = 0;

    rcu_periph_clock_enable(RCU_TIMER10);
    timer_deinit(TIMER10);

    timer_parameter_struct timer_initpara;
    timer_struct_para_init(&timer_initpara);

    timer_initpara.prescaler = TIMER_PSC_10KHZ;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = (10000 - 1); // 10KHz / 10000 = 1 Hz
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_init(TIMER10, &timer_initpara);

    timer_flag_clear(TIMER10, ~0);

    timer_interrupt_flag_clear(TIMER10, ~0);
    timer_interrupt_enable(TIMER10, TIMER_INT_UP);

    NVIC_SetPriority(TIMER0_TRG_CMT_TIMER10_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL); // Lowest priority
    NVIC_EnableIRQ(TIMER0_TRG_CMT_TIMER10_IRQn);

    timer_enable(TIMER10);
}

void PixelOutputMulti::Blackout()
{
    DEBUG_ENTRY();

    auto& pixel_configuration = PixelConfiguration::Get();

    if (pixel_configuration.IsRTZProtocol())
    {
        auto* p = reinterpret_cast<uint32_t*>(s_pixel_buffer_data);
        for (uint32_t i = 0; i < ((buffer_size_ + 1) / 2); i++)
        {
            p[i] = GPIO_PINx | (GPIO_PINx << 16);
        }
    }
    else
    {
        const auto kType = pixel_configuration.GetType();
        if ((kType == pixel::Type::APA102) || (kType == pixel::Type::SK9822) || (kType == pixel::Type::P9813))
        {
            const auto kCount = pixel_configuration.GetCount();
            for (auto port_index = 0; port_index < pixel::kPortCount; port_index++)
            {
                SetPixel4Bytes(port_index, 0, 0, 0, 0, 0);

                for (uint32_t pixel_index = 1; pixel_index <= kCount; pixel_index++)
                {
                    SetPixel4Bytes(port_index, pixel_index, 0xE0, 0, 0, 0);
                }

                if ((kType == pixel::Type::APA102) || (kType == pixel::Type::SK9822))
                {
                    SetPixel4Bytes(port_index, 1U + kCount, 0xFF, 0xFF, 0xFF, 0xFF);
                }
                else
                {
                    SetPixel4Bytes(port_index, 1U + kCount, 0, 0, 0, 0);
                }
            }
        }
        else
        {
            assert(kType == pixel::Type::WS2801);
            auto* p = reinterpret_cast<uint32_t*>(s_pixel_buffer_data);
            for (uint32_t i = 0; i < ((buffer_size_ + 1) / 2); i++)
            {
                p[i] = 0;
            }
        }
    }

    // Can be called any time. Make sure the previous transmit is ended.

    do
    {
        __DMB();
    } while (sv_is_running);

    Update();

    // A blackout may not be interrupted.

    do
    {
        __DMB();
    } while (sv_is_running);

    DEBUG_EXIT();
}

bool PixelOutputMulti::IsUpdating()
{
    __DMB();
    return sv_is_running;
}

uint32_t PixelOutputMulti::GetUserData()
{
    return sv_updates_per_second;
}

PixelOutputMulti::PixelOutputMulti()
{
    DEBUG_ENTRY();

    assert(s_this == nullptr);
    s_this = this;

    rcu_periph_clock_enable(RCU_GPIOx);
#if !defined(GD32F4XX)
    gpio_init(GPIOx, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PINx);
#else
    gpio_mode_set(GPIOx, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, GPIO_PINx);
    gpio_output_options_set(GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PINx);
#endif

    GPIO_BC(GPIOx) = GPIO_PINx;

#ifndef NDEBUG
    rcu_periph_clock_enable(DEBUG_CS_RCU_GPIOx);
#if !defined(GD32F4XX)
    gpio_init(DEBUG_CS_GPIOx, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, DEBUG_CS_GPIO_PINx);
#else
    gpio_mode_set(DEBUG_CS_GPIOx, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, DEBUG_CS_GPIO_PINx);
    gpio_output_options_set(DEBUG_CS_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, DEBUG_CS_GPIO_PINx);
#endif

    GPIO_BOP(DEBUG_CS_GPIOx) = DEBUG_CS_GPIO_PINx;
#endif

    dma::memcpy32::Init();
    Timer10Config();

    ApplyConfiguration();

    DEBUG_EXIT();
}

void PixelOutputMulti::ApplyConfiguration()
{
    DEBUG_ENTRY();

    auto& pixel_configuration = PixelConfiguration::Get();

    pixel_configuration.Validate();

    if (!pixel_configuration.RefreshNeeded())
    {
        DEBUG_EXIT();
        return;
    }

    const auto kType = pixel_configuration.GetType();
    const auto kLedsPerPixel = pixel_configuration.GetLedsPerPixel();

    if ((kType == pixel::Type::APA102) || (kType == pixel::Type::SK9822) || (kType == pixel::Type::P9813))
    {
        DEBUG_PRINTF("kMaxApA102=%u", kMaxApA102);
        if (pixel_configuration.GetCount() > kMaxApA102)
        {
            pixel_configuration.SetCount(kMaxApA102);
            pixel_configuration.Validate();
        }
    }

    const auto kCount = pixel_configuration.GetCount();
    buffer_size_ = kCount * kLedsPerPixel;

    if ((kType == pixel::Type::APA102) || (kType == pixel::Type::SK9822) || (kType == pixel::Type::P9813))
    {
        buffer_size_ += kCount;
        buffer_size_ += 8;
    }

    buffer_size_ *= 8;

    DEBUG_PRINTF("PORT_COUNT=%u, kPixelBufferSize=%u [%u]", pixel::kPortCount, kPixelBufferSize, (kPixelBufferSize + 1023) / 1024);
    DEBUG_PRINTF("s_pixel_buffer_data=%p, s_pixel_buffer_dma=%p", s_pixel_buffer_data, s_pixel_buffer_dma);
    DEBUG_PRINTF("buffer_size_=%u [%u]", buffer_size_, (buffer_size_ + 1023) / 1024);
    assert(buffer_size_ <= kPixelBufferSize);

    if (pixel_configuration.IsRTZProtocol())
    {
        Setup(pixel_configuration.GetLowCode(), pixel_configuration.GetHighCode());
    }
    else
    {
        Setup(pixel_configuration.GetClockSpeedHz());
    }

    DEBUG_EXIT();
}

void PixelOutputMulti::Setup(uint8_t low_code, uint8_t high_code)
{
    DEBUG_ENTRY();

    // Timer 2 is Master -> TIMER2_TRGO
    // Timer 3 is Slave -> ITI2

    const auto kT0H = (__builtin_popcount(low_code) * (pixel::kRtzTimerPeriod + 1)) / 8;
    const auto kT1H = (__builtin_popcount(high_code) * (pixel::kRtzTimerPeriod + 1)) / 8;

    DEBUG_PRINTF("RTZ_TIMER_PERIOD=%u, kT0H=%u, kT1H=%u", pixel::kRtzTimerPeriod, kT0H, kT1H);

    timer_parameter_struct timer_initpara;

    // Timer 2 Master
    // There is no DMA Channel for Timer 2 Channel 1

    rcu_periph_clock_enable(RCU_TIMER2);

    timer_deinit(TIMER2);

    timer_initpara.prescaler = 0;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = pixel::kRtzTimerPeriod; // 1.25 us
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;

    timer_init(TIMER2, &timer_initpara);

    timer_master_slave_mode_config(TIMER2, TIMER_MASTER_SLAVE_MODE_DISABLE);
    timer_master_output_trigger_source_select(TIMER2, TIMER_TRI_OUT_SRC_CH0);

    timer_channel_output_mode_config(TIMER2, TIMER_CH_0, TIMER_OC_MODE_ACTIVE);
    timer_channel_output_mode_config(TIMER2, TIMER_CH_2, TIMER_OC_MODE_ACTIVE);
    timer_channel_output_mode_config(TIMER2, TIMER_CH_3, TIMER_OC_MODE_ACTIVE);

    timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_0, 1); // High
    assert(kT0H < UINT16_MAX);
    timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_2, static_cast<uint16_t>(kT0H));
    assert(kT1H < UINT16_MAX);
    timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_3, static_cast<uint16_t>(kT1H));

    // Timer 3 Slave

    rcu_periph_clock_enable(RCU_TIMER3);

    timer_deinit(TIMER3);

    timer_initpara.prescaler = 0;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = static_cast<uint32_t>(~0);
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;

    timer_init(TIMER3, &timer_initpara);

    timer_master_slave_mode_config(TIMER3, TIMER_MASTER_SLAVE_MODE_DISABLE);
    timer_slave_mode_select(TIMER3, TIMER_SLAVE_MODE_EXTERNAL0);
    timer_input_trigger_source_select(TIMER3, TIMER_SMCFG_TRGSEL_ITI2);

    timer_channel_output_mode_config(TIMER3, TIMER_CH_0, TIMER_OC_MODE_ACTIVE);
    assert((1 + buffer_size_) < UINT16_MAX);
    timer_channel_output_pulse_value_config(TIMER3, TIMER_CH_0, static_cast<uint16_t>(1 + buffer_size_));

    timer_interrupt_enable(TIMER3, TIMER_INT_CH0);

    NVIC_SetPriority(TIMER3_IRQn, 0);
    NVIC_EnableIRQ(TIMER3_IRQn);

    //  END Timer's

    // START DMA configuration

    DMA_PARAMETER_STRUCT dma_init_struct;
    rcu_periph_clock_enable(TIMER2_RCU_DMAx);

    // Timer 2 Channel 0
    dma_deinit(TIMER2_DMAx, TIMER2_CH0_DMA_CHx);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
#if !defined(GD32F4XX)
    dma_init_struct.memory_addr = reinterpret_cast<uint32_t>(pixel::kSPGpioPiNs);
#else
    dma_init_struct.memory0_addr = reinterpret_cast<uint32_t>(pixel::kSPGpioPiNs);
#endif
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_DISABLE;
#if !defined(GD32F4XX)
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_16BIT;
#endif
    dma_init_struct.periph_addr = GPIOx + GPIOx_BOP_OFFSET;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if !defined(GD32F4XX)
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
#else
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
#endif
    dma_init_struct.priority = DMA_PRIORITY_LOW;
    dma_init(TIMER2_DMAx, TIMER2_CH0_DMA_CHx, &dma_init_struct);
    dma_circulation_disable(TIMER2_DMAx, TIMER2_CH0_DMA_CHx);
    dma_memory_to_memory_disable(TIMER2_DMAx, TIMER2_CH0_DMA_CHx);
#if defined(GD32F4XX)
    dma_channel_subperipheral_select(TIMER2_DMAx, TIMER2_CH0_DMA_CHx, TIMER2_CH0_DMA_SUBPERIx);
#endif

    // Timer 2 Channel 2
    dma_deinit(TIMER2_DMAx, TIMER2_CH2_DMA_CHx);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
#if !defined(GD32F4XX)
    dma_init_struct.memory_addr = reinterpret_cast<uint32_t>(s_pixel_buffer_dma);
#else
    dma_init_struct.memory0_addr = reinterpret_cast<uint32_t>(s_pixel_buffer_dma);
#endif
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if !defined(GD32F4XX)
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_16BIT;
#endif
    dma_init_struct.periph_addr = GPIOx + GPIOx_BC_OFFSET;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if !defined(GD32F4XX)
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
#else
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
#endif
    dma_init_struct.priority = DMA_PRIORITY_LOW;
    dma_init(TIMER2_DMAx, TIMER2_CH2_DMA_CHx, &dma_init_struct);
    dma_circulation_disable(TIMER2_DMAx, TIMER2_CH2_DMA_CHx);
    dma_memory_to_memory_disable(TIMER2_DMAx, TIMER2_CH2_DMA_CHx);
#if defined(GD32F4XX)
    dma_channel_subperipheral_select(TIMER2_DMAx, TIMER2_CH2_DMA_CHx, TIMER2_CH2_DMA_SUBPERIx);
#endif

    // Timer 2 Channel 3
    dma_deinit(TIMER2_DMAx, TIMER2_CH3_DMA_CHx);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
#if !defined(GD32F4XX)
    dma_init_struct.memory_addr = reinterpret_cast<uint32_t>(pixel::kSPGpioPiNs);
#else
    dma_init_struct.memory0_addr = reinterpret_cast<uint32_t>(pixel::kSPGpioPiNs);
#endif
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_DISABLE;
#if !defined(GD32F4XX)
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_16BIT;
#endif
    dma_init_struct.periph_addr = GPIOx + GPIOx_BC_OFFSET;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if !defined(GD32F4XX)
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
#else
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
#endif
    dma_init_struct.priority = DMA_PRIORITY_LOW;
    dma_init(TIMER2_DMAx, TIMER2_CH3_DMA_CHx, &dma_init_struct);
    dma_circulation_disable(TIMER2_DMAx, TIMER2_CH3_DMA_CHx);
    dma_memory_to_memory_disable(TIMER2_DMAx, TIMER2_CH3_DMA_CHx);
#if defined(GD32F4XX)
    dma_channel_subperipheral_select(TIMER2_DMAx, TIMER2_CH3_DMA_CHx, TIMER2_CH3_DMA_SUBPERIx);
#endif

    // END DMA configuration

    DEBUG_EXIT();
}

void PixelOutputMulti::Setup(uint32_t frequency)
{
    DEBUG_ENTRY();

    // BEGIN GPIO

    rcu_periph_clock_enable(TIMER2CH0_RCU_GPIOx);
#if !defined(GD32F4XX)
    rcu_periph_clock_enable(RCU_AF);
    gpio_init(TIMER2CH0_GPIOx, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, TIMER2CH0_GPIO_PINx);
#else
    gpio_mode_set(TIMER2CH0_GPIOx, GPIO_MODE_AF, GPIO_PUPD_NONE, TIMER2CH0_GPIO_PINx);
    gpio_output_options_set(TIMER2CH0_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, TIMER2CH0_GPIO_PINx);
    gpio_af_set(TIMER2CH0_GPIOx, GPIO_AF_2, TIMER2CH0_GPIO_PINx);
#endif

    // END GPIO

    // Timer 2 is Master -> TIMER2_TRGO
    // Timer 3 is Slave -> ITI2

    // Frequency = MASTER_TIMER_CLOCK  / x => x = MASTER_TIMER_CLOCK / Frequency

    auto ticker = (MASTER_TIMER_CLOCK / frequency);

    DEBUG_PRINTF("nFrequency=%u, nTicker=%u", frequency, ticker);

    if (ticker < 12)
    { // ((nTicker / 4) - 1) >= 2
        ticker = 12;
    }

    // Timer 2 Master
    // Implementation note: There is no DMA Channel for Timer 2 Channel 1

    rcu_periph_clock_enable(RCU_TIMER2);
    timer_deinit(TIMER2);

    timer_parameter_struct timer_initpara;
    timer_initpara.prescaler = 0;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = ticker - 1;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER2, &timer_initpara);

    timer_oc_parameter_struct timer_ocintpara;
    timer_ocintpara.ocpolarity = TIMER_OC_POLARITY_LOW;
    timer_ocintpara.outputstate = TIMER_CCX_ENABLE;
    timer_ocintpara.ocnpolarity = TIMER_OCN_POLARITY_HIGH;
    timer_ocintpara.outputnstate = TIMER_CCXN_DISABLE;
    timer_ocintpara.ocidlestate = TIMER_OC_IDLE_STATE_HIGH;
    timer_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;

    timer_channel_output_config(TIMER2, TIMER_CH_0, &timer_ocintpara);
    assert(((ticker / 2) - 1) < UINT16_MAX);
    timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_0, static_cast<uint16_t>((ticker / 2) - 1));
    timer_channel_output_mode_config(TIMER2, TIMER_CH_0, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER2, TIMER_CH_0, TIMER_OC_SHADOW_DISABLE);

    timer_channel_output_mode_config(TIMER2, TIMER_CH_2, TIMER_OC_MODE_ACTIVE);
    timer_channel_output_mode_config(TIMER2, TIMER_CH_3, TIMER_OC_MODE_ACTIVE);

    timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_2, 1);
    assert(((ticker / 4) - 1) >= 2);
    assert(((ticker / 4) - 1) < UINT16_MAX);
    timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_3, static_cast<uint16_t>((ticker / 4) - 1));

    timer_master_slave_mode_config(TIMER2, TIMER_MASTER_SLAVE_MODE_DISABLE);
    timer_master_output_trigger_source_select(TIMER2, TIMER_TRI_OUT_SRC_CH0);

    // Timer 3 Slave
    rcu_periph_clock_enable(RCU_TIMER3);
    timer_deinit(TIMER3);

    timer_initpara.prescaler = 0;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = static_cast<uint32_t>(~0);
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;

    timer_init(TIMER3, &timer_initpara);

    timer_master_slave_mode_config(TIMER3, TIMER_MASTER_SLAVE_MODE_DISABLE);
    timer_slave_mode_select(TIMER3, TIMER_SLAVE_MODE_EXTERNAL0);
    timer_input_trigger_source_select(TIMER3, TIMER_SMCFG_TRGSEL_ITI2);

    timer_channel_output_mode_config(TIMER3, TIMER_CH_0, TIMER_OC_MODE_ACTIVE);
    assert(buffer_size_ < UINT16_MAX);
    timer_channel_output_pulse_value_config(TIMER3, TIMER_CH_0, static_cast<uint16_t>(buffer_size_));

    timer_interrupt_enable(TIMER3, TIMER_INT_CH0);

    NVIC_SetPriority(TIMER3_IRQn, 0);
    NVIC_EnableIRQ(TIMER3_IRQn);

    // END Timer's

    // START DMA configuration

    DMA_PARAMETER_STRUCT dma_init_struct;
    rcu_periph_clock_enable(TIMER2_RCU_DMAx);

    // Timer 2 Channel 2
    dma_deinit(TIMER2_DMAx, TIMER2_CH2_DMA_CHx);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
#if !defined(GD32F4XX)
    dma_init_struct.memory_addr = reinterpret_cast<uint32_t>(pixel::kSPGpioPiNs);
#else
    dma_init_struct.memory0_addr = reinterpret_cast<uint32_t>(pixel::kSPGpioPiNs);
#endif
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_DISABLE;
#if !defined(GD32F4XX)
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_16BIT;
#endif
    dma_init_struct.periph_addr = GPIOx + GPIOx_BC_OFFSET;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if !defined(GD32F4XX)
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
#else
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
#endif
    dma_init_struct.priority = DMA_PRIORITY_LOW;
    dma_init(TIMER2_DMAx, TIMER2_CH2_DMA_CHx, &dma_init_struct);
    dma_circulation_disable(TIMER2_DMAx, TIMER2_CH2_DMA_CHx);
    dma_memory_to_memory_disable(TIMER2_DMAx, TIMER2_CH2_DMA_CHx);
#if defined(GD32F4XX)
    dma_channel_subperipheral_select(TIMER2_DMAx, TIMER2_CH2_DMA_CHx, TIMER2_CH2_DMA_SUBPERIx);
#endif

    // Timer 2 Channel 3
    dma_deinit(TIMER2_DMAx, TIMER2_CH3_DMA_CHx);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
#if !defined(GD32F4XX)
    dma_init_struct.memory_addr = reinterpret_cast<uint32_t>(s_pixel_buffer_data);
#else
    dma_init_struct.memory0_addr = reinterpret_cast<uint32_t>(s_pixel_buffer_dma);
#endif
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if !defined(GD32F4XX)
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_16BIT;
#endif
    dma_init_struct.periph_addr = GPIOx + GPIOx_BOP_OFFSET;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if !defined(GD32F4XX)
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
#else
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
#endif
    dma_init_struct.priority = DMA_PRIORITY_LOW;
    dma_init(TIMER2_DMAx, TIMER2_CH3_DMA_CHx, &dma_init_struct);
    dma_circulation_disable(TIMER2_DMAx, TIMER2_CH3_DMA_CHx);
    dma_memory_to_memory_disable(TIMER2_DMAx, TIMER2_CH3_DMA_CHx);
#if defined(GD32F4XX)
    dma_channel_subperipheral_select(TIMER2_DMAx, TIMER2_CH3_DMA_CHx, TIMER2_CH3_DMA_SUBPERIx);
#endif

    // END DMA configuration

    // BEGIN Buffer setup

    auto& pixel_configuration = PixelConfiguration::Get();
    const auto kType = pixel_configuration.GetType();

    if ((kType == pixel::Type::APA102) || (kType == pixel::Type::SK9822) || (kType == pixel::Type::P9813))
    {
        const auto kCount = pixel_configuration.GetCount();
        for (auto port_index = 0; port_index < pixel::kPortCount; port_index++)
        {
            SetPixel4Bytes(port_index, 0, 0, 0, 0, 0);

            if ((kType == pixel::Type::APA102) || (kType == pixel::Type::SK9822))
            {
                SetPixel4Bytes(port_index, 1U + kCount, 0xFF, 0xFF, 0xFF, 0xFF);
            }
            else
            {
                SetPixel4Bytes(port_index, 1U + kCount, 0, 0, 0, 0);
            }
        }
    }

    // END Buffer setup

    DEBUG_EXIT();
}

void PixelOutputMulti::FullOn()
{
    DEBUG_ENTRY();

    auto& pixel_configuration = PixelConfiguration::Get();

    if (pixel_configuration.IsRTZProtocol())
    {
        auto* p = reinterpret_cast<uint32_t*>(s_pixel_buffer_data);
        for (uint32_t i = 0; i < ((buffer_size_ + 1) / 2); i++)
        {
            p[i] = 0;
        }
    }
    else
    {
        const auto kType = pixel_configuration.GetType();
        if ((kType == pixel::Type::APA102) || (kType == pixel::Type::SK9822) || (kType == pixel::Type::P9813))
        {
            const auto kCount = pixel_configuration.GetCount();
            for (auto port_index = 0; port_index < pixel::kPortCount; port_index++)
            {
                SetPixel4Bytes(port_index, 0, 0, 0, 0, 0);

                for (uint32_t pixel_index = 1; pixel_index <= kCount; pixel_index++)
                {
                    SetPixel4Bytes(port_index, pixel_index, 0xE0, 0xFF, 0xFF, 0xFF);
                }

                if ((kType == pixel::Type::APA102) || (kType == pixel::Type::SK9822))
                {
                    SetPixel4Bytes(port_index, 1U + kCount, 0xFF, 0xFF, 0xFF, 0xFF);
                }
                else
                {
                    SetPixel4Bytes(port_index, 1U + kCount, 0, 0, 0, 0);
                }
            }
        }
        else
        {
            assert(kType == pixel::Type::WS2801);
            auto* p = reinterpret_cast<uint32_t*>(&s_pixel_buffer_data[0]);
            for (uint32_t i = 0; i < ((buffer_size_ + 1) / 2); i++)
            {
                p[i] = 0xFF;
            }
        }
    }

    // Can be called any time. Make sure the previous transmit is ended.

    do
    {
        __DMB();
    } while (sv_is_running);

    Update();

    // May not be interrupted.

    do
    {
        __DMB();
    } while (sv_is_running);

    DEBUG_EXIT();
}

#pragma GCC push_options
#pragma GCC optimize("O3")

void PixelOutputMulti::Update()
{
    do
    { // https://github.com/vanvught/rpidmx512/issues/281
        __ISB();
    } while (sv_is_running);

    sv_is_running = true;

    auto timer3_ctl0 = TIMER_CTL0(TIMER3);
    timer3_ctl0 &= ~TIMER_CTL0_CEN;
    TIMER_CTL0(TIMER3) = timer3_ctl0;
    TIMER_CNT(TIMER3) = 0;

    auto timer2_ctl0 = TIMER_CTL0(TIMER2);
    timer2_ctl0 &= ~TIMER_CTL0_CEN;
    TIMER_CTL0(TIMER2) = timer2_ctl0;
    TIMER_CNT(TIMER2) = 0;

#if defined(DMA_MEMCPY32_DISABLE_IRQ)
    dma::memcpy32::StartDma(reinterpret_cast<uint8_t*>(s_pixel_buffer_dma), reinterpret_cast<uint8_t*>(s_pixel_buffer_data), buffer_size_ / 2);
#endif

    const auto& pixel_configuration = PixelConfiguration::Get();

    if (pixel_configuration.IsRTZProtocol())
    {
        uint32_t chctl_ch0 = DMA_CHCTL(TIMER2_DMAx, TIMER2_CH0_DMA_CHx);
        chctl_ch0 &= ~DMA_CHXCTL_CHEN;
        DMA_CHCTL(TIMER2_DMAx, TIMER2_CH0_DMA_CHx) = chctl_ch0;
        Gd32DmaInterruptFlagClear<TIMER2_DMAx, TIMER2_CH0_DMA_CHx, DMA_INTF_FTFIF>();
        DMA_CHMADDR(TIMER2_DMAx, TIMER2_CH0_DMA_CHx) = reinterpret_cast<uint32_t>(pixel::kSPGpioPiNs);
        DMA_CHCNT(TIMER2_DMAx, TIMER2_CH0_DMA_CHx) = (buffer_size_ & DMA_CHXCNT_CNT);
        chctl_ch0 |= DMA_CHXCTL_CHEN;
        DMA_CHCTL(TIMER2_DMAx, TIMER2_CH0_DMA_CHx) = chctl_ch0;

        uint32_t chctl_ch2 = DMA_CHCTL(TIMER2_DMAx, TIMER2_CH2_DMA_CHx);
        chctl_ch2 &= ~DMA_CHXCTL_CHEN;
        DMA_CHCTL(TIMER2_DMAx, TIMER2_CH2_DMA_CHx) = chctl_ch2;
        Gd32DmaInterruptFlagClear<TIMER2_DMAx, TIMER2_CH2_DMA_CHx, DMA_INTF_FTFIF>();
        DMA_CHMADDR(TIMER2_DMAx, TIMER2_CH2_DMA_CHx) = reinterpret_cast<uint32_t>(s_pixel_buffer_dma);
        DMA_CHCNT(TIMER2_DMAx, TIMER2_CH2_DMA_CHx) = ((buffer_size_)&DMA_CHXCNT_CNT);
        chctl_ch2 |= DMA_CHXCTL_CHEN;
        DMA_CHCTL(TIMER2_DMAx, TIMER2_CH2_DMA_CHx) = chctl_ch2;

        uint32_t chctl_ch3 = DMA_CHCTL(TIMER2_DMAx, TIMER2_CH3_DMA_CHx);
        chctl_ch3 &= ~DMA_CHXCTL_CHEN;
        DMA_CHCTL(TIMER2_DMAx, TIMER2_CH3_DMA_CHx) = chctl_ch3;
        Gd32DmaInterruptFlagClear<TIMER2_DMAx, TIMER2_CH3_DMA_CHx, DMA_INTF_FTFIF>();
        DMA_CHMADDR(TIMER2_DMAx, TIMER2_CH3_DMA_CHx) = reinterpret_cast<uint32_t>(pixel::kSPGpioPiNs);
        DMA_CHCNT(TIMER2_DMAx, TIMER2_CH3_DMA_CHx) = ((buffer_size_)&DMA_CHXCNT_CNT);
        chctl_ch3 |= DMA_CHXCTL_CHEN;
        DMA_CHCTL(TIMER2_DMAx, TIMER2_CH3_DMA_CHx) = chctl_ch3;

        TIMER_DMAINTEN(TIMER2) |= (TIMER_DMA_CH0D | TIMER_DMA_CH2D | TIMER_DMA_CH3D);
    }
    else
    {
        uint32_t chctl_ch2 = DMA_CHCTL(TIMER2_DMAx, TIMER2_CH2_DMA_CHx);
        chctl_ch2 &= ~DMA_CHXCTL_CHEN;
        DMA_CHCTL(TIMER2_DMAx, TIMER2_CH2_DMA_CHx) = chctl_ch2;
        Gd32DmaInterruptFlagClear<TIMER2_DMAx, TIMER2_CH2_DMA_CHx, DMA_INTF_FTFIF>();
        DMA_CHMADDR(TIMER2_DMAx, TIMER2_CH2_DMA_CHx) = reinterpret_cast<uint32_t>(pixel::kSPGpioPiNs);
        DMA_CHCNT(TIMER2_DMAx, TIMER2_CH2_DMA_CHx) = (buffer_size_ & DMA_CHXCNT_CNT);
        chctl_ch2 |= DMA_CHXCTL_CHEN;
        DMA_CHCTL(TIMER2_DMAx, TIMER2_CH2_DMA_CHx) = chctl_ch2;

        uint32_t chctl_ch3 = DMA_CHCTL(TIMER2_DMAx, TIMER2_CH3_DMA_CHx);
        chctl_ch3 &= ~DMA_CHXCTL_CHEN;
        DMA_CHCTL(TIMER2_DMAx, TIMER2_CH3_DMA_CHx) = chctl_ch3;
        Gd32DmaInterruptFlagClear<TIMER2_DMAx, TIMER2_CH3_DMA_CHx, DMA_INTF_FTFIF>();
        DMA_CHMADDR(TIMER2_DMAx, TIMER2_CH3_DMA_CHx) = reinterpret_cast<uint32_t>(s_pixel_buffer_dma);
        DMA_CHCNT(TIMER2_DMAx, TIMER2_CH3_DMA_CHx) = (buffer_size_ & DMA_CHXCNT_CNT);
        chctl_ch3 |= DMA_CHXCTL_CHEN;
        DMA_CHCTL(TIMER2_DMAx, TIMER2_CH3_DMA_CHx) = chctl_ch3;

        TIMER_DMAINTEN(TIMER2) |= (TIMER_DMA_CH2D | TIMER_DMA_CH3D);
    }

#if defined(DMA_MEMCPY32_DISABLE_IRQ)
    while (dma::memcpy32::IsActive())
    {
    }

    TIMER_CTL0(TIMER3) |= TIMER_CTL0_CEN;
    TIMER_CTL0(TIMER2) |= TIMER_CTL0_CEN;
#else
    dma::memcpy32::StartDma(reinterpret_cast<uint8_t*>(s_pixel_buffer_dma), reinterpret_cast<uint8_t*>(s_pixel_buffer_data), buffer_size_ / 2);
#endif

#ifndef NDEBUG
    GPIO_BC(DEBUG_CS_GPIOx) = DEBUG_CS_GPIO_PINx;
#endif

    sv_updates = sv_updates + 1;
}

extern "C"
{
    void TIMER3_IRQHandler()
    { // Slave
        const auto kIntFlag = TIMER_INTF(TIMER3);

        if ((kIntFlag & TIMER_INT_FLAG_CH0) == TIMER_INT_FLAG_CH0)
        {
            TIMER_CTL0(TIMER2) &= (~TIMER_CTL0_CEN);

            TIMER_DMAINTEN(TIMER2) &= (~(TIMER_DMA_CH0D | TIMER_DMA_CH2D | TIMER_DMA_CH3D));

            GPIO_BC(GPIOx) = GPIO_PINx;
#ifndef NDEBUG
            GPIO_BOP(DEBUG_CS_GPIOx) = DEBUG_CS_GPIO_PINx;
#endif
            sv_is_running = false;
        }

        TIMER_INTF(TIMER3) = ~kIntFlag;
    }

    void TIMER0_TRG_CMT_TIMER10_IRQHandler()
    { // 1 Hz interrupt
        const auto kIntFlag = TIMER_INTF(TIMER10);

        if ((kIntFlag & TIMER_INT_FLAG_UP) == TIMER_INT_FLAG_UP)
        {
            sv_updates_per_second = sv_updates - sv_updates_previous;
            sv_updates_previous = sv_updates;
        }

        TIMER_INTF(TIMER10) = ~kIntFlag;
    }

#if !defined(DMA_MEMCPY32_DISABLE_IRQ)
#if !defined(GD32F4XX)
    void DMA0_Channel3_IRQHandler()
    { // DMX memcpy ready
        if (Gd32DmaInterruptFlagGet<DMA0, DMA_CH3, DMA_INT_FLAG_FTF>())
        {
            Gd32DmaInterruptDisable<DMA0, DMA_CH3, (DMA_INT_FTF | DMA_INT_HTF | DMA_INT_ERR)>();

            TIMER_CTL0(TIMER3) |= TIMER_CTL0_CEN;
            TIMER_CTL0(TIMER2) |= TIMER_CTL0_CEN;
        }

        Gd32DmaInterruptFlagClear<DMA0, DMA_CH3, (DMA_INT_FLAG_FTF | DMA_INT_FLAG_G)>();
    }
#else
    void DMA1_Channel0_IRQHandler()
    { // DMX memcpy ready
        if (Gd32DmaInterruptFlagGet<DMA1, DMA_CH0, DMA_INT_FLAG_FTF>())
        {
            Gd32DmaInterruptDisable<DMA1, DMA_CH0, (DMA_CHXCTL_FTFIE | DMA_CHXCTL_HTFIE | DMA_CHXFCTL_FEEIE)>();

            TIMER_CTL0(TIMER3) |= TIMER_CTL0_CEN;
            TIMER_CTL0(TIMER2) |= TIMER_CTL0_CEN;
        }

        Gd32DmaInterruptFlagClear<DMA1, DMA_CH0, (DMA_INT_FLAG_FTF | DMA_INT_FLAG_TAE)>();
    }
#endif
#endif
}
