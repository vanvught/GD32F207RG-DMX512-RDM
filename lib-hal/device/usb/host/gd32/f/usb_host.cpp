/**
 * usb_host.cpp
 *
 */
/* Copyright (C) 2023-2024 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include <cstdint>

#include "gd32.h"

/**
 * Needed for older GD32F firmware
 */
#if !defined(GPIO_OSPEED_MAX)
# define GPIO_OSPEED_MAX	GPIO_OSPEED_200MHZ
#endif

extern "C" {
#include "usbh_core.h"
#include "usbh_msc_core.h"
#include "drv_usbh_int.h"
void usb_mdelay(uint32_t);
}

static void usb_gpio_config() {
#if !defined (GD32F4XX)
#else
    rcu_periph_clock_enable(RCU_SYSCFG);
    rcu_periph_clock_enable(RCU_GPIOA);

    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_11 | GPIO_PIN_12);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_11 | GPIO_PIN_12);

    gpio_af_set(GPIOA, GPIO_AF_10, GPIO_PIN_11 | GPIO_PIN_12);
#endif
}

#if !defined (GD32F4XX)
__IO uint32_t usbfs_prescaler = 0U;
__IO uint16_t timer_prescaler = 5U;
#else
#endif

static void usb_rcu_config() {
#if !defined (GD32F4XX)
    uint32_t system_clock = rcu_clock_freq_get(CK_SYS);

    if (48000000U == system_clock) {
        timer_prescaler = 3U;
        usbfs_prescaler = RCU_CKUSB_CKPLL_DIV1;
    } else if (72000000U == system_clock) {
        timer_prescaler = 5U;
        usbfs_prescaler = RCU_CKUSB_CKPLL_DIV1_5;
    } else if (96000000U == system_clock) {
        timer_prescaler = 7U;
        usbfs_prescaler = RCU_CKUSB_CKPLL_DIV2;
    } else if (120000000U == system_clock) {
        timer_prescaler = 9U;
        usbfs_prescaler = RCU_CKUSB_CKPLL_DIV2_5;
    }  else {
        /* reserved */
    }

    rcu_usbfs_trng_clock_config(usbfs_prescaler);
    rcu_periph_clock_enable(RCU_USBFS);
#else
    rcu_pll48m_clock_config(RCU_PLL48MSRC_PLLQ);
    rcu_ck48m_clock_config(RCU_CK48MSRC_PLL48M);

    rcu_periph_clock_enable(RCU_USBFS);
#endif
}

static void usb_intr_config() {
    nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);
    nvic_irq_enable((uint8_t)USBFS_IRQn, 2U, 0U);

#if !defined (GD32F4XX)
#else
#endif
}

void usb_vbus_config () {
#if defined (USB_HOST_VBUS_GPIOx)
    rcu_periph_clock_enable(USB_HOST_VBUS_RCU_GPIOx);

# if defined (GPIO_INIT)
    gpio_init(USB_HOST_VBUS_GPIOx, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, USB_HOST_VBUS_GPIO_PINx);
# else
    gpio_mode_set(USB_HOST_VBUS_GPIOx, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, USB_HOST_VBUS_GPIO_PINx);
    gpio_output_options_set(USB_HOST_VBUS_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, USB_HOST_VBUS_GPIO_PINx);
# endif

    gpio_bit_set(USB_HOST_VBUS_GPIOx, USB_HOST_VBUS_GPIO_PINx);

    usb_mdelay (200);
#endif
}

#if !defined (GD32F4XX)
extern usb_core_driver usbh_core;
usbh_host usb_host;
#else
usb_core_driver usbh_core;
usbh_host usb_host_msc;
#endif
extern usbh_user_cb usr_cb;

void usb_init() {
    usb_gpio_config();
    usb_rcu_config();
	usb_vbus_config();
#if !defined (GD32F4XX)
    usbh_class_register(&usb_host, &usbh_msc);
    usbh_init (&usb_host, &usr_cb);
#else
    usbh_class_register(&usb_host_msc, &usbh_msc);
    usbh_init(&usb_host_msc,
              &usbh_core,
              USB_CORE_ENUM_FS,
              &usr_cb);
#endif

    usb_intr_config ();
}

extern "C" {
void USBFS_IRQHandler(void) {
	usbh_isr (&usbh_core);
}
}
