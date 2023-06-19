#ifndef __EDITOR_H__
#define __EDITOR_H__

#include <stdint.h>

void minied_main(uint8_t* tx_buff, uint8_t* rx_buff);

// TODO: can do better than this
typedef struct {
	uint8_t line1[128];
	uint8_t line2[128];
	uint8_t line3[128];
	uint8_t line4[128];
} minied_buff_t;


#endif