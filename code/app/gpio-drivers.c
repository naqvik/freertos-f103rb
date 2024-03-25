#include <assert.h>
#include <stdint.h>

/* HW-specific includes (move to bsp area) */
#include "stm32f10x.h"

#include "gpio-drivers.h"

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

void gpio_config_pin(GPIO_TypeDef* base, uint32_t pin, uint32_t bits4) {
    assert(base != ((void*)0));
    assert(pin < 16);
    assert(bits4 < 15);  // must be a valid pattern from table

    Reg32 CR =  (pin >= 8) ? &base->CRH : &base->CRL;
    pin  = (pin >= 8) ? pin - 8 : pin;
    *CR &= ~(0xfu << (pin*4));  // zero the nybble
    *CR |= bits4 << (pin*4);    // assign bits4 to nybble
}

void gpio_pin_onoff(GPIO_TypeDef* base, uint32_t pin, bool on) {
    // Require: pin must be already configured as output
    assert(pin < 16);

    //uint32_t mask = 1u << pin;
    if (on) {
        base->BSRR = 1u << pin;
    } else {
        base->BSRR = 1u << (pin+16u);
    }
}

