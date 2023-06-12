#include <stdint.h>

volatile uint32_t s_ticks;

void hardfault_handler(void) {
	while(1);
}

void systick_handler(void) {
	s_ticks++;
}