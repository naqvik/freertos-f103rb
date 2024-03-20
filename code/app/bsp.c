/**
   bsp.c: Board Support Package for nucleo-f103rb
*/

#include <assert.h>
#include <stm32f10x.h>
#include "bsp.h"


void NVIC_set_enable(uint32_t irq_num) {
    // the f103rb only supports IRQ# 0-68 (or 0-0x44)
    //assert(irq_num < 68);

    // irq_num has a certain structure
    // 31 30 29 28  ...   9  8  7  6  5  4  3  2  1  0
    //+--+--+--+--+     +--+--+--+--+--+--+--+--+--+--+
    //|  |  |  |  | ... |  |  |  |  |  |  |  |  |  |  |
    //+--+--+--+--+     +--+--+--+--+--+--+--+--+--+--+
    //                    --------------->|<--------->|
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
