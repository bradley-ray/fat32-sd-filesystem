#include "hal.h"
#include <stddef.h>

extern volatile uint32_t s_ticks;

void system_init(void) {
	// systick init for 1ms tick rate
	STK_CSR |= BIT(2) + BIT(1);
	STK_RVR = 12000-1;
	STK_CVR |= 1;
}

void delay(uint32_t ms) {
	s_ticks = 0;
	STK_CSR |= BIT(0);
	while(s_ticks < ms);
	STK_CSR &= ~BIT(0);
	STK_CVR |= 1;
}

/* GPIO Defintions */
void gpio_set_mode(gpio_struct_t* gpio, uint32_t pin, uint32_t mode) {
    gpio->MODER &= ~((uint32_t)0x03 << (2*pin));
    gpio->MODER |= mode << (2*pin);
}

void gpio_set_output_type(gpio_struct_t* gpio, uint32_t pin, uint32_t mode) {
    gpio->OTYPER &= ~BIT(5);
    gpio->OTYPER |= mode << pin;
}

void gpio_set_speed(gpio_struct_t* gpio, uint32_t pin, uint32_t mode) {
    gpio->OSPEEDR &= ~((uint32_t)0x03 << (2*pin));
    gpio->OSPEEDR |= mode << (2*pin);
}

void gpio_set_pull(gpio_struct_t* gpio, uint32_t pin, uint32_t mode) {
    gpio->PUPDR &= ~((uint32_t)0x03 << (2*pin));
    gpio->PUPDR |= mode << (2*pin);
}

void gpio_set(gpio_struct_t* gpio, uint32_t pin) {
    gpio->ODR |= BIT(pin);
}
void gpio_reset(gpio_struct_t* gpio, uint32_t pin) {
    gpio->ODR &= ~BIT(pin);
}

void gpio_toggle(gpio_struct_t* gpio, uint32_t pin) {
    gpio->ODR ^= BIT(pin);
}

/* UART Defintions */
void uart2_write_byte(uint8_t byte) {
	USART2->TDR = (uint32_t)byte;
	while(!(USART2->ISR & BIT(7)));
}

void uart2_write_buff(uint8_t* buff, uint32_t size) {
	for (uint32_t i = 0; i < size; ++i)
		uart2_write_byte(buff[i]);
	while(!(USART2->ISR & BIT(6)));
}

uint8_t uart2_read_byte(void) {
	while(!(USART2->ISR & BIT(5)));
	return (uint8_t)USART2->RDR;
}

void uart2_read_buff(uint8_t* buff, uint32_t size) {
	for (uint32_t i = 0; i < size; ++i)
		buff[i] = uart2_read_byte();
}

uint8_t uart2_read_buff_until(uint8_t eof, uint8_t* buff, uint32_t size) {
	for(uint32_t i = 0; i < size; ++i) {
		buff[i] = uart2_read_byte();
		if (buff[i] == eof)
			return 1;
	}
	return 0;
}

uint8_t spi_read_write_byte(uint8_t byte) {
	while(!(SPI1->SR & BIT(1)));
	*((volatile uint8_t*)&(SPI1->DR)) = byte;
	while(!(SPI1->SR & BIT(0)));
	return *((volatile uint8_t*)&(SPI1->DR));
}

void spi_read_write_buff(uint8_t* tx_buff, uint8_t* rx_buff, uint32_t size) {
	uint8_t response;
	for (uint32_t i = 0; i < size; ++i) {
		response = spi_read_write_byte(tx_buff[i]);
		if (response)
			rx_buff[i] = response;	
	}
}
