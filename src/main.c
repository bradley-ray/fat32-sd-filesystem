#include <stdio.h>
#include "hal.h"
#include "sd.h"
#include "fat32.h"
#include "helpers.h"

#define FREQ 12000000
#define LED_TOGGLE() gpio_toggle(GPIOA, 5)
#define NSS 4

void gpio_led_init(void);
void uart2_init(uint32_t baud);
void spi_init(void);

// General buffers
uint8_t tx_buff[512];
uint8_t rx_buff[512];

int main() {
	uart2_init(115200);
	(void)uart2_read_byte();
	spi_init();

	delay(10);

	sd_preinit();
	SPI1->CR2 |= BIT(2);
	sd_init();
	fat_init(rx_buff);

	// want to move this to seperaate function
	while(1) {	
		if (uart2_read_buff_until('\n', rx_buff, 512)) {
			uint8_t cmd_size = 0;
			uint8_t arg_size = 0;
			for(uint32_t i = 0; i < 512 && rx_buff[i] != '\n' && rx_buff[i] != ' '; ++i, ++cmd_size);

			if (str_eq((uint8_t*)"ls", 2, rx_buff, cmd_size)) {
				list_dir(rx_buff);
			} else if (str_eq((uint8_t*)"cd", 2, rx_buff, cmd_size)) {
				if (rx_buff[cmd_size] == '\n') {
					change_dir("root", rx_buff);
					continue;
				}
				for(uint32_t i = cmd_size+1; i < 512 && rx_buff[i] != '\n'; ++i, ++arg_size);
				rx_buff[cmd_size+1+arg_size] = 0;
				change_dir((char*)(rx_buff+cmd_size+1), rx_buff);
			} else if (str_eq((uint8_t*)"mkdir", 5, rx_buff, cmd_size)) {
				if (rx_buff[cmd_size] == '\n') {
					printf("invalid argument\n");
					continue;
				}
				for(uint32_t i = cmd_size+1; i < 512 && rx_buff[i] != '\n'; ++i, ++arg_size);
				rx_buff[cmd_size+1+arg_size] = 0;
				make_dir((char*)(rx_buff+cmd_size+1), rx_buff);
			} else if (str_eq((uint8_t*)"cat", 3, rx_buff, cmd_size)) {
				if (rx_buff[cmd_size] == '\n') {
					printf("invalid argument\n");
					continue;
				}
				for(uint32_t i = cmd_size+1; i < 512 && rx_buff[i] != '\n'; ++i, ++arg_size);
				rx_buff[cmd_size+1+arg_size] = 0;
				cat_file((char*)(rx_buff+cmd_size+1), rx_buff);
			} else if (str_eq((uint8_t*)"ed", 2, rx_buff, cmd_size)) {
				edit_file(tx_buff, rx_buff);
			} else if (str_eq((uint8_t*)"pwd", 3, rx_buff, cmd_size)) {
				print_current_dir();
			} else if (str_eq((uint8_t*)"touch", 5, rx_buff, cmd_size)) {
				if (rx_buff[cmd_size] == '\n') {
					printf("invalid argument\n");
					continue;
				}
				for(uint32_t i = cmd_size+1; i < 512 && rx_buff[i] != '\n'; ++i, ++arg_size);
				rx_buff[cmd_size+1+arg_size] = 0;
				create_file((char*)(rx_buff+cmd_size+1), rx_buff);
			} else if (str_eq((uint8_t*)"rm", 2, rx_buff, cmd_size)) {
				if (rx_buff[cmd_size] == '\n') {
					printf("invalid argument\n");
					continue;
				}
				for(uint32_t i = cmd_size+1; i < 512 && rx_buff[i] != '\n'; ++i, ++arg_size);
				rx_buff[cmd_size+1+arg_size] = 0;
				delete_file((char*)(rx_buff+cmd_size+1), rx_buff);
			} else if (str_eq((uint8_t*)"rmdir", 5, rx_buff, cmd_size)) {
				if (rx_buff[cmd_size] == '\n') {
					printf("invalid argument\n");
					continue;
				}
				for(uint32_t i = cmd_size+1; i < 512 && rx_buff[i] != '\n'; ++i, ++arg_size);
				rx_buff[cmd_size+1+arg_size] = 0;
				delete_dir((char*)(rx_buff+cmd_size+1), rx_buff);
			} else {
				printf("unknown command...\n");
			}
		}

		printf("-----------------\n");
		fat_cache_fat(rx_buff);
	}
}

void gpio_led_init(void) {
	RCC_ENABLE_CLK_GPIOA();
	gpio_set_mode(GPIOA, 5, GPIO_MODE_OUTPUT);
	gpio_set_output_type(GPIOA, 5, GPIO_OUT_TYPE_PP);
	gpio_set_speed(GPIOA, 5, GPIO_SPEED_VERY_HIGH);
	gpio_set_pull(GPIOA, 5, GPIO_PULL_NONE);
}

void uart2_init(uint32_t baud) {
	RCC_ENABLE_CLK_USART2();
	USART2_SET_BAUD(FREQ,baud);
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
	gpio_set_mode(GPIOA, NSS, GPIO_MODE_AF);
	gpio_set_output_type(GPIOA, NSS, GPIO_OUT_TYPE_OD);
	gpio_set_pull(GPIOA, NSS, GPIO_PULL_UP);
	gpio_set_speed(GPIOA, NSS, GPIO_SPEED_VERY_LOW);
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