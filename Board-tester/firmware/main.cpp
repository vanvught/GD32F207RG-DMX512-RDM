#include "hardware.h"
#include "ledblink.h"

extern "C" {
#include "gd32f20x.h"
#include "gd32f20x_gpio.h"
#include "gd32f20x_rcu.h"

void uart0_puts(const char *);
void uart0_putc(int);

// Skip not used PINs and skip USART0
static constexpr auto GPIOA_PINS = ~(GPIO_PIN_0 | GPIO_PIN_3 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10);
static constexpr auto GPIOB_PINS = ~(GPIO_PIN_1);
static constexpr auto GPIOC_PINS = ~(GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_13);
static constexpr auto GPIOD_PINS = GPIO_PIN_2;

int main(void) {
	Hardware hw;
	LedBlink lb;

	uart0_puts("Board tester\nAll GPIO's are set to output HIGH");

	uart0_puts("rcu_periph_clock_enable");
	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_clock_enable(RCU_GPIOB);
	rcu_periph_clock_enable(RCU_GPIOC);
	rcu_periph_clock_enable(RCU_GPIOD);
	rcu_periph_clock_enable(RCU_AF);

	uart0_puts("gpio_init");
	gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIOA_PINS);
	gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIOB_PINS);
	gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIOC_PINS);
	gpio_init(GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIOD_PINS);

	gpio_pin_remap_config(GPIO_SWJ_DISABLE_REMAP, ENABLE);

	uart0_puts("GPIO_BOP");
	GPIO_BOP(GPIOA) = GPIOA_PINS;
	GPIO_BOP(GPIOB) = GPIOB_PINS;
	GPIO_BOP(GPIOC) = GPIOC_PINS;
	GPIO_BOP(GPIOD) = GPIOD_PINS;

	uart0_puts("Done!");

	while (1) {
		lb.Run();
	}
}

}
