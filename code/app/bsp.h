/**
   bsp.h: Interface to board Support Package for nucleo-f103rb

   To the breadboard I added external LEDs as follows:

                                  510
    [D6, PB10]--[RedLED]--[2]-----vvvvv-----[1]---[GND]
    [D7,  PA8]--[YlwLED]--[4]-----vvvvv-----[1]---[GND]
    [D8,  PA9]--[GrnLED]--[6]-----vvvvv-----[1]---[GND]
    [D10, PB6]--[BluLED]--[8]-----vvvvv-----[1]---[GND]

    Resistor array, 9 RES
    510 Ohm, 10SIP                    Internal structure
    +-------------------------+    [1]-----+-----+--...---+
    |  CTSK1949770101511P     |            |     |        |
    |   1 2 3 4 5 6 7 8 9 10  |            \     \        \
    +-+-+-+-+-+-+-+-+-+-+-+-+-+            /     /        /
        | | | | | | | | | |             510\  510\ ... 510\
        1 2 3 4 5 6 7 8 9 10               |     |        |
                                          [2]   [3]     [10]
*/
#ifndef BSP_H
#define BSP_H

#include <stdint.h>
#include <stm32f10x.h>

void NVIC_set_enable(uint32_t irq_num);

// clock enabling-disabling functions can be inlined

void enable_afio_clk(void) {
    RCC->APB2ENR |= (1u << 0); // bits[0] = AFIOEN <- 1
}
void enable_gpioa_clk(void) {
    RCC->APB2ENR |= (1u << 2); // bits[2] = IOPAEN <- 1
}
void enable_gpiob_clk(void) {
    RCC->APB2ENR |= (1u << 3); // bits[3] = IOPAEN <- 1
}
void enable_gpioc_clk(void) {
    RCC->APB2ENR |= (1u << 4); // bits[4] = IOPCEN <- 1
}
void enable_gpiod_clk(void) {
    RCC->APB2ENR |= (1u << 5); // bits[5] = IOPCEN <- 1
}
#endif // BSP_H
