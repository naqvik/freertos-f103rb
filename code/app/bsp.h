/** -*- c++ -*-
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
#include <stdbool.h>
#include <stm32f10x.h>

// used for range-checking input parameters
typedef enum { PortA, PortB, PortC, PortD, PortE
} Port;
typedef enum {
    Pin0, Pin1, Pin2, Pin3, Pin4, Pin5, Pin6, Pin7,
    Pin8, Pin9, Pin10, Pin11, Pin12, Pin13, Pin14, Pin15,
} Pin;

// NVIC-related functions
void NVIC_set_enable(uint32_t irq_num);
void NVIC_clr_pending(uint32_t irq_num);

void afio_exticr_source(Port port, Pin pin);

// Many of these function can be inlined

// clock enabling-disabling functions
static inline void enable_afio_clk(void) {
    RCC->APB2ENR |= (1u << 0); // bits[0] = AFIOEN <- 1
}
static inline void enable_gpioa_clk(void) {
    RCC->APB2ENR |= (1u << 2); // bits[2] = IOPAEN <- 1
}
static inline void enable_gpiob_clk(void) {
    RCC->APB2ENR |= (1u << 3); // bits[3] = IOPAEN <- 1
}
static inline void enable_gpioc_clk(void) {
    RCC->APB2ENR |= (1u << 4); // bits[4] = IOPCEN <- 1
}
static inline void enable_gpiod_clk(void) {
    RCC->APB2ENR |= (1u << 5); // bits[5] = IOPCEN <- 1
}

// External event/interrupt configuration
static inline void exti_unmask(uint32_t line, bool unmask) {
    assert(line==13);
    if (unmask)
        EXTI->IMR |= (1u << line); // bits[line] <- 1
    else
        EXTI->IMR &= ~(1u << line); // bits[line] <- 0
}

static inline void exti_falling_edge_trig(uint32_t line, bool enable) {
    assert(line==13);
    if (enable)
        EXTI->FTSR |= (1u << line);
    else
        EXTI->FTSR &= ~(1u << line);
}
static inline void exti_rising_edge_trig(uint32_t line, bool enable) {
    assert(line==13);
    if (enable)
        EXTI->RTSR |= (1u << line);
    else
        EXTI->RTSR &= ~(1u << line);
}

// introduce global variable, shared between an ISR and a thread
extern uint32_t gl_button_count;

#endif // BSP_H
