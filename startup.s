.syntax unified

.global _reset
.global	_vectors
.global	default_handler

@ defined in link.ld
.word _sidata
.word _sdata
.word _edata
.word _sbss
.word _ebss

.section .text.reset
.type _reset, %function
_reset:
    ldr R0,=_sbss                   @ initialize .bss section to all zeros
    ldr R1,=_ebss                    
    movs R2,#0
zero_bss:                           
    cmp R0,R1
    bcs copy_data
    str R2,[R0]
    adds R0,#4
    b zero_bss
copy_data:                          
    ldr R0,=_sdata                  @ copy .data from flash to ram
    ldr R1,=_edata
    ldr R2,=_sidata
loop_data:
    cmp R0,R1
    bcs copy_done
    ldr R3,[R2]
    str R3,[R0]
    adds R2,#4
    adds R0,#4
    b loop_data
copy_done:
    ldr R0,=system_init
    blx R0
    ldr R0,=main         
    blx R0                          @ call main()
infinite_loop:                      @ if main returns run infinte loop
    b infinite_loop

@ irq vector table
.section .vectors,"a",%progbits
.type _vectors, %object
.size _vectors, .-_vectors
_vectors:
    .word _estack                   @ top of stack
    .word _reset
    .word nmi_handler
    .word hardfault_handler
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word svcall_handler            @ triggered by SVC (OS use SVC to access kernel functions)
    .word 0
    .word 0
    .word pendsv_handler            @ request for system-level service (context swiching in OS)
    .word systick_handler

		@ stm32c0 interrupts
		.word wwdg_irq_handler
		.word 0
		.word rtc_irq_handler
		.word flash_irq_handler
		.word rcc_irq_handler
		.word exti0_1_irq_handler
		.word exti2_3_irq_handler
		.word exti4_15_irq_handler
		.word 0
		.word dma1_channel1_irq_handler
		.word dma1_channel2_3_irq_handler
		.word dmamux_irq_handler
		.word adc_irq_handler
		.word tim1_brk_up_trg_com_irq_handler
		.word tim1_cc_irq_handler
		.word 0
		.word tim3_irq_handler
		.word 0
		.word 0
		.word tim14_irq_handler
		.word 0
		.word tim16_irq_handler
		.word tim17_irq_handler
		.word i2c1_irq_handler
		.word 0
		.word spi1_irq_handler
		.word 0
		.word usart1_irq_handler
		.word usart2_irq_handler
		.word 0
		.word 0
		.word 0

.section .text.default_handler,"ax",%progbits
default_handler:
    b default_handler

.weak nmi_handler
.thumb_set nmi_handler,default_handler
.weak hardfault_handler
.thumb_set hardfault_handler,default_handler
.weak svcall_handler
.thumb_set svcall_handler,default_handler
.weak pendsv_handler
.thumb_set pendsv_handler,default_handler
.weak systick_handler
.thumb_set systick_handler,default_handler
.weak wwdg_irq_handler
.thumb_set wwdg_irq_handler,default_handler
.weak rtc_irq_handler
.thumb_set rtc_irq_handler,default_handler
.weak flash_irq_handler
.thumb_set flash_irq_handler,default_handler
.weak rcc_irq_Handler
.thumb_set rcc_irq_handler,default_handler
.weak exti0_1_irq_handler
.thumb_set exti0_1_irq_handler,default_handler
.weak exti2_3_irq_handler
.thumb_set exti2_3_irq_handler,default_handler
.weak exti4_15_irq_handler
.thumb_set exti4_15_irq_handler,default_handler
.weak dma1_channel1_irq_handler
.thumb_set dma1_channel1_irq_handler,default_handler
.weak dma1_channel2_3_irq_handler
.thumb_set dma1_channel2_3_irq_handler,default_handler
.weak dmamux_irq_handler
.thumb_set dmamux_irq_handler,default_handler
.weak adc_irq_handler
.thumb_set adc_irq_handler,default_handler
.weak tim1_brk_up_trg_com_irq_handler
.thumb_set tim1_brk_up_trg_com_irq_handler,default_handler
.weak tim1_cc_irq_handler
.thumb_set tim1_cc_irq_handler,default_handler
.weak tim3_irq_handler
.thumb_set tim3_irq_handler,default_handler
.weak tim14_irq_handler
.thumb_set tim14_irq_handler,default_handler
.weak tim16_irq_handler
.thumb_set tim16_irq_handler,default_handler
.weak tim17_irq_handler
.thumb_set tim17_irq_handler,default_handler
.weak i2c1_irq_handler
.thumb_set i2c1_irq_handler,default_handler
.weak spi1_irq_handler
.thumb_set spi1_irq_handler,default_handler
.weak usart1_irq_handler
.thumb_set usart1_irq_handler,default_handler
.weak usart2_irq_handler
.thumb_set usart2_irq_handler,default_handler
