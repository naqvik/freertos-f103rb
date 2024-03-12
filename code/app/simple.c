// -*- c++ -*-
/** A simple app, to demonstrate freertos */

/* standard includes */
#include <stdio.h>
#include <stdbool.h>

/* freertos includes */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

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

/*  Added external LEDs as follows:

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


typedef uint32_t volatile * const Reg32;
void gpio_config_pin(GPIO_TypeDef* base, uint32_t pin, uint32_t bits4);

void gpio_config_pin(GPIO_TypeDef* base, uint32_t pin, uint32_t bits4) {
    configASSERT(base != ((void*)0));
    configASSERT(pin < 16);
    configASSERT(bits4 < 15);  // must be a valid pattern from table

    Reg32 CR =  (pin >= 8) ? &base->CRH : &base->CRL;
    pin  = (pin >= 8) ? pin - 8 : pin;
    *CR &= ~(0xfu << (pin*4));  // zero the nybble
    *CR |= bits4 << (pin*4);    // assign bits4 to nybble
}

void gpio_pin_onoff(GPIO_TypeDef* base, uint32_t pin, bool on);
void gpio_pin_onoff(GPIO_TypeDef* base, uint32_t pin, bool on) {
    // Require: pin must be already configured as output
    configASSERT(pin < 16);

    //uint32_t mask = 1u << pin;
    if (on) {
        base->BSRR = 1u << pin;
    } else {
        base->BSRR = 1u << (pin+16u);
    }
}


////////////////////////////////////////////////////////////////
// Allow hand-off from task blinkPA8 to blinkPA5.  I want the PA8 task
// to finish its work *before* the PA5 task runs.  This semaphore is
// used to enforce this.  The PA8 task's period is 1000 ms, while
// the PA5 task's period is 200 ms.  When a falling edge occurs on
// PA8, the PA5 task is released, runs to completion, then
// blocks until PA8 emits another falling edge.
//
//  PA5....................^^__................^^__.......... etc
//  PA8__________^^^^^^^^^^__________^^^^^^^^^^__________^^^^^ etc
static SemaphoreHandle_t gl_sequence_tasks_sem = ((void*)0);

__attribute__((noreturn)) static void blinkPA5(void * blah) {
    (void) blah;
    // turn on clock for GPIOA
    RCC->APB2ENR |= 1<<2;

    // configure PA5 to be output, push-pull, 50MHz
    gpio_config_pin(GPIOA, 5u, 3u);

    while (1) {
        xSemaphoreTake(gl_sequence_tasks_sem, portMAX_DELAY);  // wait

        // turn on PA5 LED
        gpio_pin_onoff(GPIOA, 5, 1);
        vTaskDelay(100);

        // turn off PA5 LED
        gpio_pin_onoff(GPIOA, 5, 0);
        vTaskDelay(100);
    }
    //return 0;
}
__attribute__((noreturn)) static void displayPattern(void * blah) {
    (void) blah;
    // turn on clock for GPIOA and GPIOB
    RCC->APB2ENR |= 1u<<2;
    RCC->APB2ENR |= 1u<<3;

    // configure four pins to be output, push-pull, 50MHz
    // PB10: Red, PA8: Yellow, PA9: Green, PB6: Blue
    gpio_config_pin(GPIOB, 10u, 3u);
    gpio_config_pin(GPIOA, 8u, 3u);
    gpio_config_pin(GPIOA, 9u, 3u);
    gpio_config_pin(GPIOB, 6u, 3u);

    struct {
        GPIO_TypeDef* gpio;
        uint32_t pin;
        uint32_t onoff;
    } seq[]  = {
        {GPIOB, 10, 1},  // turn on PB10
        {GPIOB, 10, 0},  // turn off PB10
        {GPIOA, 8, 1},   // turn on PA8 LED
        {GPIOA, 8, 0},   // turn off PA8 LED
        {GPIOA, 9, 1},   // turn on PA9 LED
        {GPIOA, 9, 0},   // turn off PA9 LED
        {GPIOB, 6, 1},   // turn on PB4 LED
        {GPIOB, 6, 0},   // turn off PB4 LED
    };

    while (1) {
        for (uint32_t i=0; i < 8; ++i) {
            GPIO_TypeDef* gpio = seq[i].gpio;
            uint32_t pin = seq[i].pin;
            uint32_t onoff = seq[i].onoff;

            gpio_pin_onoff(gpio, pin, onoff);
            vTaskDelay(500);
        }
    }
    //return 0;
}
int main() {
    BaseType_t retval = xTaskCreate(
        blinkPA5,    // task function
        "blink PA5", // task name
        50,          // stack in words
        ((void*)0),     // optional parameter
        4,           // priority
        ((void*)0)      // optional out: task handle
        );
    configASSERT(retval==pdPASS);

    retval = xTaskCreate(
        displayPattern,    // task function
        "blink PA8", // task name
        50,          // stack in words
        ((void*)0),     // optional parameter
        4,           // priority
        ((void*)0)      // optional out: task handle
        );
    configASSERT(retval==pdPASS);

    gl_sequence_tasks_sem = xSemaphoreCreateBinary();
    configASSERT(gl_sequence_tasks_sem != ((void*)0));

    vTaskStartScheduler();
}

typedef struct ae {
    char const * filename;
    int32_t lin_num;
} AssertionError;

static uint32_t const BUF_SIZE = 32;
//static AssertionError buffer[BUF_SIZE] = {0};


typedef struct cb {
    AssertionError buffer[BUF_SIZE];
    uint32_t head;
    uint32_t tail;
} CBuffer;

static CBuffer cbuffer = { buffer, 0, 0};

void cbuffer_insert(AssertionError const* ae) {
    cbuffer.buffer[cbuffer.tail] = *ae;
    cbuffer.tail = (cbuffer.tail+1) % BUF_SIZE;
}

/**
   I changed the original function body (which did a busy-wait on a
   global variable 'ul') to a circular buffer that stores the filename
   and line number where the assertion was triggered.

   Now you can, inside the debugger, view the entries in the buffer
   and see the line numbers and files where assertions were triggered.

   The function now simply returns instead of spinning, which is
   undesireable for certain errors.  Need to add behaviour: spinlock
   on 'emergencies' and just update cbuffer on lower-level conditions.
 */
void vAssertCalled(char const * const filename, int line_num) {
    // FIXME assertmutex take

    AssertionError ae = {filename, line_num};
    cbuffer_insert(ae);

    // FIXME assertmutex give
}
