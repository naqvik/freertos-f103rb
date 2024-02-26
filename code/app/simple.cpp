/** A simple app, to demonstrate freertos */

/* standard includes */

/* freertos includes */
#include "FreeRTOS.h"
#include "task.h"

/* HW-specific includes (move to bsp area) */
#include "stm32f10x.h"

/**
   BLink the LED, using the lowest-level code possible
*/
#include <stdint.h>
// Where is the green LED?  connected to PB13 or PA5 with a 510 Ohm resistor
//   It looks like it's actually PA5, based on the note on p66.
//   So we need to put a High/Low on PA5 to turn LD2 On/Off.
// So we need to use GPIO port A, bit 5
// RCC block starts at 0x4002 1000
// APB2ENR is at RCC + 0x18, set bit 2
// base of GPIOA is 0x4001 0800 (is also CRL)
// want to set pin 5 of the ODR, so we need to configure pin 5
//  using CRL register
// The ODR of GPIOA is at GPIOA base address + 12 (decimal)

/*

   Full table of all possible CNF[1:0]MODE[1:0] patterns, and their
   meanings.  GPI=General Purpose Input, GPO=General Purpose Output,
   AFO=Alternate Function Output.

   |      |      |       |       | PxODR |                         |
   | CNF1 | CNF0 | MODE1 | MODE0 | bit   | Meaning                 |
   |------+------+-------+-------+-------+-------------------------|
   |    0 |    0 |     0 |     0 | x     | GPI, analog             |
   |    0 |    0 |     0 |     1 | 0/1   | GPO, push-pull, 10MHz   |
   |    0 |    0 |     1 |     0 | 0/1   | GPO, push-pull, 2MHz    |
   |    0 |    0 |     1 |     1 | 0/1   | GPO, push-pull, 50MHz   |
   |------+------+-------+-------+-------+-------------------------|
   |    0 |    1 |     0 |     0 | x     | GPI, floating (default) |
   |    0 |    1 |     0 |     1 | 0/1   | GPO, open-drain, 10MHz  |
   |    0 |    1 |     1 |     0 | 0/1   | GPO, open-drain, 2MHz   |
   |    0 |    1 |     1 |     1 | 0/1   | GPO, open-drain, 50MHz  |
   |------+------+-------+-------+-------+-------------------------|
   |    1 |    0 |     0 |     0 | 0/1   | GPI, pulldown/pullup    |
   |    1 |    0 |     0 |     1 | x     | AFO, push-pull, 10MHz   |
   |    1 |    0 |     1 |     0 | x     | AFO, push-pull, 2MHz    |
   |    1 |    0 |     1 |     1 | x     | AFO, push-pull, 50MHz   |
   |------+------+-------+-------+-------+-------------------------|
   |    1 |    1 |     0 |     0 | x     | forbidden               |
   |    1 |    1 |     0 |     1 | x     | AFO, open-drain, 10MHz  |
   |    1 |    1 |     1 |     0 | x     | AFO, open-drain, 2MHz   |
   |    1 |    1 |     1 |     1 | x     | AFO, open-drain, 50MHz  |
   |------+------+-------+-------+-------+-------------------------|

*/
using Reg32 = uint32_t volatile * const;

void gpio_config_pin(GPIO_TypeDef* base, uint32_t pin, uint32_t bits4) {
    configASSERT(base != nullptr);
    configASSERT(pin < 16);
    configASSERT(bits4 < 15);  // must be a valid pattern from table

    Reg32 CR =  (pin >= 8) ? &base->CRH : &base->CRL;
    pin  = (pin >= 8) ? pin - 8 : pin;
    *CR &= ~(0xfu << (pin*4));  // zero the nybble
    *CR |= bits4 << (pin*4);    // assign bits4 to nybble
}

[[noreturn]] static void blinkPA5(void * blah) {
    (void) blah;
    // turn on clock for GPIOA
    //*((uint32_t volatile *)0x40021018) |= 4;
    RCC->APB2ENR |= 1<<2;

    // configure PA5 to be output, push-pull, 50MHz
    //*((uint32_t volatile *)(0x40010800 + 0)) = 0x44344444;
    // GPIOA->CRL &= ~(0xfu << (5*4));
    // GPIOA->CRL |= 3u << (5*4);
    gpio_config_pin(GPIOA, 5u, 3u);

    while (1) {
        // turn on PA5 LED
        *((uint32_t volatile *)(0x40010800 + 0xc)) |=  1u<<5;
        for (int volatile counter = 0; counter < 1000000; ++counter) { }

        // turn off PA5 LED
        *((uint32_t volatile *)(0x40010800 + 0xc)) &= ~(1u<<5);
        for (int volatile counter = 0; counter < 1000000; ++counter) { }
    }
    //return 0;
}
[[noreturn]] static void blinkPA8(void * blah) {
    (void) blah;
    // turn on clock for GPIOA
    //*((uint32_t volatile *)0x40021018) |= 4;
    RCC->APB2ENR |= 1<<2;

    // configure PA8 to be output, push-pull, 50MHz
    //*((uint32_t volatile *)(0x40010800 + 4)) = 0x44444443;
    GPIOA->CRH &= ~(0xfu << (0*4));
    GPIOA->CRH |= 0x3u << (0*4);

    while (1) {
        // turn on PA8 LED
        *((uint32_t volatile *)(0x40010800 + 0xc)) |=  1u<<8;
        for (int volatile counter = 0; counter < 1000000; ++counter) { }

        // turn off PA8 LED
        *((uint32_t volatile *)(0x40010800 + 0xc)) &= ~(1u<<8);
        for (int volatile counter = 0; counter < 1000000; ++counter) { }
    }
    //return 0;
}
int main() {
    BaseType_t retval = xTaskCreate(
        blinkPA5,    // task function
        "blink PA5", // task name
        50,          // stack in words
        nullptr,     // optional parameter
        4,           // priority
        nullptr      // optional out: task handle
        );
    configASSERT(retval==pdPASS);

    retval = xTaskCreate(
        blinkPA8,    // task function
        "blink PA8", // task name
        50,          // stack in words
        nullptr,     // optional parameter
        4,           // priority
        nullptr      // optional out: task handle
        );
    configASSERT(retval==pdPASS);

    vTaskStartScheduler();
}
