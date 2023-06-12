#ifndef __HAL_H__
#define __HAL_H__

#include <inttypes.h>

#define BIT(n) ((uint32_t)(1 << (n)))

/* RCC Registers */
typedef struct {
    volatile uint32_t CR, ICSCR, CFGR, RESERVED1, RESERVED2, RESERVED3, CIER, CIFR,
                    CICR, IOPRSTR, AHBRSTR, APBRSTR1, APBRSTR2, IOPENR, AHBENR, APBENR1,
                    APBENR2, IOPSMENR, AHBSMENR, APBSMENR1, APBSMENR2, CCIPR, CSR1, CSR2;
} rcc_struct_t;
#define RCC ((rcc_struct_t*)0x40021000)

/* RCC Helpers */
#define RCC_ENABLE_CLK_GPIOA() RCC->IOPENR |= BIT(0)
#define RCC_ENABLE_CLK_GPIOB() RCC->IOPENR |= BIT(1)
#define RCC_ENABLE_CLK_GPIOC() RCC->IOPENR |= BIT(2)
#define RCC_ENABLE_CLK_GPIOD() RCC->IOPENR |= BIT(3)
#define RCC_ENABLE_CLK_GPIOF() RCC->IOPENR |= BIT(5)

/* SysTick Registers */
#define STK_CSR *((volatile uint32_t*) (0xE000E010))
#define STK_RVR *((volatile uint32_t*) (0xE000E014))
#define STK_CVR *((volatile uint32_t*) (0xE000E018))
#define STK_CALIB *((volatile uint32_t*) (0xE000E01C))

void delay(uint32_t ms);

/* GPIO Registers */
typedef struct {
    volatile uint32_t MODER, OTYPER, OSPEEDR, PUPDR, IDR, ODR, BSRR, LCKR, AFRL, AFRH, BRR;
} gpio_struct_t;
#define GPIOA ((gpio_struct_t*)0x50000000)
#define GPIOB ((gpio_struct_t*)0x50000400)
#define GPIOC ((gpio_struct_t*)0x50000800)
#define GPIOD ((gpio_struct_t*)0x50000C00)
#define GPIOF ((gpio_struct_t*)0x50001400)

/* GPIO Helpers */
#define GPIO_AF0 ((uint32_t)0x0)
#define GPIO_AF1 ((uint32_t)0x1)
#define GPIO_AF2 ((uint32_t)0x2)
#define GPIO_AF3 ((uint32_t)0x3)
#define GPIO_AF4 ((uint32_t)0x4)
#define GPIO_AF5 ((uint32_t)0x5)
#define GPIO_AF6 ((uint32_t)0x6)
#define GPIO_AF7 ((uint32_t)0x7)
#define GPIO_AF8 ((uint32_t)0x8)
#define GPIO_AF9 ((uint32_t)0x9)
#define GPIO_AF10 ((uint32_t)0xa)
#define GPIO_AF11 ((uint32_t)0xb)
#define GPIO_AF12 ((uint32_t)0xc)
#define GPIO_AF13 ((uint32_t)0xd)
#define GPIO_AF14 ((uint32_t)0xe)
#define GPIO_AF15 ((uint32_t)0xf)

#define GPIO_MODE_INPUT 0x00
#define GPIO_MODE_OUTPUT 0x01
#define GPIO_MODE_AF 0x02
#define GPIO_MODE_ANALOG 0x03
void gpio_set_mode(gpio_struct_t* gpio, uint32_t pin, uint32_t mode);

#define GPIO_OUT_TYPE_PP 0x00
#define GPIO_OUT_TYPE_OD 0x01
void gpio_set_output_type(gpio_struct_t* gpio, uint32_t pin, uint32_t mode);

#define GPIO_SPEED_VERY_LOW 0x00
#define GPIO_SPEED_LOW 0x01
#define GPIO_SPEED_HIGH 0x02
#define GPIO_SPEED_VERY_HIGH 0x03
void gpio_set_speed(gpio_struct_t* gpio, uint32_t pin, uint32_t mode);

#define GPIO_PULL_NONE 0x00
#define GPIO_PULL_UP 0x01
#define GPIO_PULL_DOWN 0x02
#define GPIO_PULL_RESERVED 0x03
void gpio_set_pull(gpio_struct_t* gpio, uint32_t pin, uint32_t mode);

void gpio_set(gpio_struct_t* gpio, uint32_t pin);
void gpio_reset(gpio_struct_t* gpio, uint32_t pin);
void gpio_toggle(gpio_struct_t* gpio, uint32_t pin);

/* USART Helpers */
typedef struct {
    volatile uint32_t CR1, CR2, CR3, BRR, GTPR, RTOR, RQR, ISR, ICR, RDR, TDR, PRESC;
} usart2_struct_t;
#define USART2 ((usart2_struct_t*)0x40004400)

#define RCC_ENABLE_CLK_USART2() RCC->APBENR1 |= BIT(17)
#define USART2_SET_BAUD(freq, baud) USART2->BRR |= freq/baud
#define USART2_ENABLE() USART2->CR1 |= BIT(0)
#define USART2_ENABLE_TRANSMIT() USART2->CR1 |= BIT(3)
#define USART2_ENABLE_RECEIVE() USART2->CR1 |= BIT(2)

void uart2_write_byte(uint8_t byte);
void uart2_write_buff(uint8_t* byte, uint32_t size);
uint8_t uart2_read_byte();
void uart2_read_buff(uint8_t* byte, uint32_t size);
uint8_t uart2_read_buff_until(uint8_t eof, uint8_t* buff, uint32_t size);

/* SPI Helpers */
typedef struct {
    volatile uint32_t CR1, CR2, SR, DR, CRCPR, RXCRCR, TXCRCR, I2SCFGR, I2SPR;
} spi_struct_t;
#define SPI1 ((spi_struct_t*)0x40013000)

#define RCC_ENABLE_CLK_SPI1() RCC->APBENR2 |= BIT(12)

void spi_write_byte(uint8_t byte);
void spi_write_buff(uint8_t* buff, uint32_t size);
uint8_t spi_read_byte(void);
void spi_read_buff(uint8_t* buff, uint32_t size);
uint8_t spi_read_write_byte(uint8_t byte);
void spi_read_write_buff(uint8_t* tx_buff, uint8_t* rx_buff, uint32_t size);

#endif