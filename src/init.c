#include "init.h"
#include "hal.h"

void gpio_led_init(void) {
	RCC_ENABLE_CLK_GPIOA();
	gpio_set_mode(GPIOA, 5, GPIO_MODE_OUTPUT);
	gpio_set_output_type(GPIOA, 5, GPIO_OUT_TYPE_PP);
	gpio_set_speed(GPIOA, 5, GPIO_SPEED_VERY_HIGH);
	gpio_set_pull(GPIOA, 5, GPIO_PULL_NONE);
}

void uart2_init(uint32_t baud) {
	RCC_ENABLE_CLK_USART2();
	USART2_SET_BAUD(DEVICE_FREQ, baud);
	USART2_ENABLE();
	USART2_ENABLE_TRANSMIT();
	USART2_ENABLE_RECEIVE();

	RCC_ENABLE_CLK_GPIOA();
	// setup GPIO 2
	gpio_set_mode(GPIOA, 2, GPIO_MODE_AF);
	gpio_set_output_type(GPIOA, 2, GPIO_OUT_TYPE_OD);
	gpio_set_pull(GPIOA, 2, GPIO_PULL_UP);
	gpio_set_speed(GPIOA, 2, GPIO_SPEED_VERY_LOW);
	GPIOA->AFRL |= (uint32_t)1 << 8;
	
	// setup GPIO 3
	gpio_set_mode(GPIOA, 3, GPIO_MODE_AF);
	gpio_set_output_type(GPIOA, 3, GPIO_OUT_TYPE_OD);
	gpio_set_pull(GPIOA, 3, GPIO_PULL_UP);
	gpio_set_speed(GPIOA, 3, GPIO_SPEED_VERY_LOW);
	GPIOA->AFRL |= (uint32_t)1 << 12;
}

// TODO: at some point switch to dma or interrupt instead of polling
void spi_init(void) {
	RCC_ENABLE_CLK_SPI1();

	// setup SCK (PA5)
	gpio_set_mode(GPIOA, 5, GPIO_MODE_AF);
	gpio_set_output_type(GPIOA, 5, GPIO_OUT_TYPE_PP);
	gpio_set_pull(GPIOA, 5, GPIO_PULL_NONE);
	gpio_set_speed(GPIOA, 5, GPIO_SPEED_VERY_LOW);
	GPIOA->AFRL |= GPIO_AF0 << 20;

	// setup MISO (PA6)
	gpio_set_mode(GPIOA, 6, GPIO_MODE_AF);
	gpio_set_output_type(GPIOA, 6, GPIO_OUT_TYPE_OD);
	gpio_set_pull(GPIOA, 6, GPIO_PULL_UP);
	gpio_set_speed(GPIOA, 6, GPIO_SPEED_VERY_LOW);
	GPIOA->AFRL |= GPIO_AF0 << 24;

	// setup MOSI (PA7)
	gpio_set_mode(GPIOA, 7, GPIO_MODE_AF);
	gpio_set_output_type(GPIOA, 7, GPIO_OUT_TYPE_PP);
	gpio_set_pull(GPIOA, 7, GPIO_PULL_NONE);
	gpio_set_speed(GPIOA, 7, GPIO_SPEED_VERY_LOW);
	GPIOA->AFRL |= GPIO_AF0 << 28;

	// setup NSS (PA4)
	gpio_set_mode(GPIOA, SPI_NSS, GPIO_MODE_AF);
	gpio_set_output_type(GPIOA, SPI_NSS, GPIO_OUT_TYPE_OD);
	gpio_set_pull(GPIOA, SPI_NSS, GPIO_PULL_UP);
	gpio_set_speed(GPIOA, SPI_NSS, GPIO_SPEED_VERY_LOW);
	GPIOA->AFRL |= GPIO_AF0 << 16;

	// MSB first
	SPI1->CR1 &= ~BIT(7);
	// fclk / 64
	SPI1->CR1 |= (uint32_t)5 << 3;

	// SPI mode 0
	SPI1->CR1 &= ~BIT(1);
	SPI1->CR1 &= ~BIT(0);

	// Master mode
	SPI1->CR1 |= BIT(2);	

	// RNXE set FIFO >= 8bit
	SPI1->CR2 |= BIT(12);
	// 8 bit data
	SPI1->CR2 |= (uint32_t)7 << 8;

	// spi ENABLE
	SPI1->CR1 |= BIT(6);
}