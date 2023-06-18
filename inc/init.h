#ifndef __INIT_H__
#define __INIT_H__

#include <stdint.h>

#define DEVICE_FREQ 48000000
#define SPI_NSS 4

void gpio_led_init(void);
void uart2_init(uint32_t baud);
void spi_init(void);

#endif