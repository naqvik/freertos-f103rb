/** A simple app, to demonstrate freertos */

/* standard includes */

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

/*  Added external yellow LED as follows:

                                  510
    [CN9-8, PA8]--[LED]--[2]-----vvvvv-----[1]---[GND]

    Resistor array, 9 RES 510 Ohm, 10SIP

    +-----------------------+    [1]-----+-----+--...---+  Yes, this is a hack
    |  CTSK1949770101511P   |            |     |        |  until I can find a
    |   1 2 3 4 5 6 7 8 9   |            \     \        \  discrete 510 Ohm
    +-+-+-+-+-+-+-+-+-+-+-+-+            /     /        /  resistor.
        | | | | | | | | |             510\  510\ ... 510\
        1 2 3 4 5 6 7 8 9                |     |        |
                                        [2]   [3]      [9]
 */



using Reg32 = uint32_t volatile * const;
void gpio_config_pin(GPIO_TypeDef* base, uint32_t pin, uint32_t bits4);

void gpio_config_pin(GPIO_TypeDef* base, uint32_t pin, uint32_t bits4) {
    configASSERT(base != nullptr);
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

    uint32_t mask = 1u << pin;
    if (on) {
        base->ODR |= mask;
    } else {
        base->ODR &= ~mask;
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
static SemaphoreHandle_t gl_sequence_tasks_sem = nullptr;

[[noreturn]] static void blinkPA5(void * blah) {
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
[[noreturn]] static void blinkPA8(void * blah) {
    (void) blah;
    // turn on clock for GPIOA
    RCC->APB2ENR |= 1<<2;

    // configure PA8 to be output, push-pull, 50MHz
    gpio_config_pin(GPIOA, 8u, 3u);

    while (1) {
        // turn on PA8 LED
        gpio_pin_onoff(GPIOA, 8, 1);
        vTaskDelay(500);

        // turn off PA8 LED
        gpio_pin_onoff(GPIOA, 8, 0);
        xSemaphoreGive(gl_sequence_tasks_sem);  // release PA5 task
        vTaskDelay(500);
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

    gl_sequence_tasks_sem = xSemaphoreCreateBinary();
    configASSERT(gl_sequence_tasks_sem != nullptr);

    vTaskStartScheduler();
}

void vAssertCalled(char const * const filename, int line_num) {
{
    uint32_t volatile ul = 0u;

    (void) filename;
    (void) line_num;
	taskENTER_CRITICAL();
	{
		// Set ul to a non-zero value using the debugger to step out
		// of this function.
		while( ul == 0 ) {
			portNOP();
		}
	}
	taskEXIT_CRITICAL();
}
