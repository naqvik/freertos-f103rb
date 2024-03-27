/**
   bsp.c: Board Support Package for nucleo-f103rb
*/

#include <assert.h>
#include <stm32f10x.h>
#include "bsp.h"

// introduce global variable, shared between an ISR and a thread
uint32_t gl_button_count = 0u;

// prototype for external ISR function used here
void EXTI15_10_IRQHandler(void);

void afio_exticr_source(Port port, Pin pin) {
    assert(0 <= port && port < 5);  // how to avoid magic number 5 here?
    assert(0 <= pin && pin < 16);

    uint32_t idx = pin / 4;  // was constrained to 0..15, now 0..3
    uint32_t nybble = pin % 4;
    AFIO->EXTICR[idx] = (uint32_t)port << (nybble*4); // bits[idx] <- port;
}

void NVIC_set_enable(uint32_t irq_num) {
    // the f103rb only supports IRQ# 0-68 (or 0-0x44)
    assert(irq_num < 68);

    // irq_num has a certain structure
    // 31 30 29 28  ...   9  8  7  6  5  4  3  2  1  0
    //+--+--+--+--+     +--+--+--+--+--+--+--+--+--+--+
    //|  |  |  |  | ... |  |  |  |  |  |  |  |  |  |  |
    //+--+--+--+--+     +--+--+--+--+--+--+--+--+--+--+
    //                    ------------>|<------------>|
    // bits[:5] select the desired register
    uint32_t idx = irq_num >> 5;

    // bit[4:0] select the bit number in the desired register
    uint32_t bit_num = irq_num & 0x1F; 

    // Writing to the ISER registers must be protected by a memory
    // barrier, before and after
    __asm volatile("":::"memory");
    NVIC->ISER[idx] = 1u << bit_num;
    __asm volatile("":::"memory");
}

// Handle USER button interrupt.  The name for this is determined
// by searching through the startup assembly code
void EXTI15_10_IRQHandler(void) {
    EXTI->PR |= (1u << 13);
    gl_button_count++;
}

void NVIC_clr_pending(uint32_t irq_num) {
    // the f103rb only supports IRQ# 0-68 (or 0-0x44)
    assert(irq_num < 68);

    // bits[:5] select the desired register
    uint32_t idx = irq_num >> 5;

    // bit[4:0] select the bit number in the desired register
    uint32_t bit_num = irq_num & 0x1F; 

    NVIC->ICPR[idx] = 1u << bit_num;
}
