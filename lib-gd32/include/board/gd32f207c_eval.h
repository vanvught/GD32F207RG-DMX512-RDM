/**
 * @file gd32f207c_eval.h
 *
 */
/* Copyright (C) 2021-2022 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef BOARD_GD32F207C_EVAL_H
#define BOARD_GD32F207C_EVAL_H

#if !defined(BOARD_GD32F207C_EVAL)
# error This file should not be included
#endif

#if defined (MCU_GD32F20X_H_)
# error This file should be included later
#endif

/**
 * LEDs
 */

#define LED1_GPIO_PINx		GPIO_PIN_0
#define LED1_GPIOx			GPIOC
#define LED1_RCU_GPIOx		RCU_GPIOC

#define LED2_GPIO_PINx		GPIO_PIN_2
#define LED2_GPIOx			GPIOC
#define LED2_RCU_GPIOx		RCU_GPIOC

#define LED3_GPIO_PINx		GPIO_PIN_0
#define LED3_GPIOx			GPIOE
#define LED3_RCU_GPIOx		RCU_GPIOE

#define LED4_GPIO_PINx		GPIO_PIN_1
#define LED4_GPIOx			GPIOE
#define LED4_RCU_GPIOx		RCU_GPIOE

#define LED_BLINK_PIN       LED1_GPIO_PINx
#define LED_BLINK_GPIO_PORT LED1_GPIOx
#define LED_BLINK_GPIO_CLK	LED1_RCU_GPIOx

/**
 * LEDs bit-banging 595
 */

#if !defined (CUSTOM_BOARD)
// Using SPI2 pin's: MOSI, SCK and NSS
#define LED595_DATA_GPIO_PINx	GPIO_PIN_5
#define LED595_DATA_RCU_GPIOx	RCU_GPIOB
#define LED595_DATA_GPIOx		GPIOB

#define LED595_CLK_GPIO_PINx	GPIO_PIN_3
#define LED595_CLK_RCU_GPIOx	RCU_GPIOB
#define LED595_CLK_GPIOx		GPIOB

#define LED595_LOAD_GPIO_PINx	GPIO_PIN_15
#define LED595_LOAD_RCU_GPIOx	RCU_GPIOA
#define LED595_LOAD_GPIOx		GPIOA
#else
#define LED595_DATA_GPIO_PINx	GPIO_PIN_2
#define LED595_DATA_RCU_GPIOx	RCU_GPIOE
#define LED595_DATA_GPIOx		GPIOE

#define LED595_CLK_GPIO_PINx	GPIO_PIN_3
#define LED595_CLK_RCU_GPIOx	RCU_GPIOE
#define LED595_CLK_GPIOx		GPIOE

#define LED595_LOAD_GPIO_PINx	GPIO_PIN_4
#define LED595_LOAD_RCU_GPIOx	RCU_GPIOE
#define LED595_LOAD_GPIOx		GPIOE

#ifdef __cplusplus
namespace hal {
namespace panelled {
static constexpr uint32_t ACTIVITY = 0x00380000;
// DMX
static constexpr uint32_t PORT_A_RX = 0x1;
static constexpr uint32_t PORT_A_TX = 0x2;
//
static constexpr uint32_t INVERTED = ACTIVITY;
}  // namespace panelled
}  // namespace hal
#endif
#endif

/**
 * KEYs
 */

#define KEY1_PINx			GPIO_PIN_0
#define KEY1_GPIOx			GPIOA
#define KEY1_RCU_GPIOx		RCU_GPIOA

#define KEY2_PINx			GPIO_PIN_13
#define KEY2_GPIOx			GPIOC
#define KEY2_RCU_GPIOx		RCU_GPIOC

#define KEY3_PINx			GPIO_PIN_14
#define KEY3_GPIOx			GPIOB
#define KEY3_RCU_GPIOx		RCU_GPIOB

#if !defined (CUSTOM_BOARD)
# define KEY_BOOTLOADER_TFTP_GPIO_PINx  KEY3_PINx
# define KEY_BOOTLOADER_TFTP_GPIOx      KEY3_GPIOx
# define KEY_BOOTLOADER_TFTP_RCU_GPIOx  KEY3_RCU_GPIOx
#else
# define KEY_BOOTLOADER_TFTP_GPIO_PINx  GPIO_PIN_3
# define KEY_BOOTLOADER_TFTP_RCU_GPIOx  RCU_GPIOD
# define KEY_BOOTLOADER_TFTP_GPIOx      GPIOD
#endif

/**
 * I2C
 */

#define I2C0_REMAP
#if defined (I2C0_REMAP)
# define I2C_REMAP			GPIO_I2C0_REMAP
#endif
#define I2C_PERIPH			I2C0_PERIPH
#define I2C_RCU_CLK			I2C0_RCU_CLK
#define I2C_GPIO_SCL_PORT	I2C0_SCL_GPIOx
#define I2C_GPIO_SCL_CLK	I2C0_SCL_RCU_GPIOx
#define I2C_GPIO_SDA_PORT	I2C0_SDA_GPIOx
#define I2C_GPIO_SDA_CLK	I2C0_SDA_RCU_GPIOx
#define I2C_SCL_PIN			I2C0_SCL_GPIO_PINx
#define I2C_SDA_PIN			I2C0_SDA_GPIO_PINx

/**
 * SPI
 */

#define SPI2_REMAP
#if defined (SPI2_REMAP)
# define SPI_REMAP			SPI2_REMAP_GPIO
#endif
#define SPI_PERIPH			SPI2_PERIPH
#define SPI_NSS_GPIOx		SPI2_NSS_GPIOx
#define SPI_NSS_RCU_GPIOx	SPI2_NSS_RCU_GPIOx
#define SPI_NSS_GPIO_PINx	SPI2_NSS_GPIO_PINx
#define SPI_RCU_CLK			SPI2_RCU_CLK
#define SPI_GPIOx			SPI2_GPIOx
#define SPI_RCU_GPIOx		SPI2_RCU_GPIOx
#define SPI_SCK_PIN			SPI2_SCK_GPIO_PINx
#define SPI_MISO_PIN		SPI2_MISO_GPIO_PINx
#define SPI_MOSI_PIN		SPI2_MOSI_GPIO_PINx
#define SPI_DMAx			SPI2_DMAx
#define SPI_DMA_CHx			SPI2_TX_DMA_CHx

/**
 * U(S)ART
 */

// #define USART0_REMAP
#define USART1_REMAP
#define USART2_FULL_REMAP
// #define USART2_PARTIAL_REMAP
// #define UART3_REMAP
// #define USART5_REMAP
// #define UART6_REMAP

#include "mcu/gd32f20x_mcu.h"
#include "gd32_gpio.h"

#define GD32_MCU_NAME			"GD32F207"

#define GD32_BOARD_NAME			"GD32F207C_EVAL"
#define GD32_BOARD_LED1			GD32_PORT_TO_GPIO(GD32_GPIO_PORTC, 0)
#define GD32_BOARD_LED2			GD32_PORT_TO_GPIO(GD32_GPIO_PORTC, 2)
#define GD32_BOARD_LED3			GD32_PORT_TO_GPIO(GD32_GPIO_PORTE, 0)
#define GD32_BOARD_LED4			GD32_PORT_TO_GPIO(GD32_GPIO_PORTE, 1)
#define GD32_BOARD_STATUS_LED	GD32_BOARD_LED1

#include "gpio_header.h"

#endif /* BOARD_GD32F207C_EVAL_H */
