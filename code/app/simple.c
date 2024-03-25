// -*- c++ -*-
/** A simple app, to demonstrate freertos */

/* standard includes */
#include <stdio.h>
#include <ctype.h>              // for isprint()
#include <assert.h>
#include <stdbool.h>

/* freertos includes */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// project includes
#include "serial-io.h"
#include "widget.h"
#include "error.h"
#include "gpio-drivers.h"       // FIXME: should not need this here

#include "bsp.h"

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

__attribute__((noreturn))
static void blinkPA5(void * blah) {
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
__attribute__((noreturn))
static void displayPattern(void * blah) {
    (void) blah;

    configureWidget();

    while (1) {
        printf("USER button count: %d\n", gl_button_count);
        runWidget();
        xSemaphoreGive(gl_sequence_tasks_sem);  // let other task run
    }
}
int main() {
    openUsart2();
    configureButton();

    BaseType_t retval = xTaskCreate(
        blinkPA5,    // task function
        "blink PA5", // task name
        50,          // stack in words
        ((void*)0),     // optional parameter
        4,           // priority
        ((void*)0)      // optional out: task handle
        );
    assert(retval==pdPASS);

    retval = xTaskCreate(
        displayPattern,    // task function
        "blink PA8", // task name
        50,          // stack in words
        ((void*)0),     // optional parameter
        4,           // priority
        ((void*)0)      // optional out: task handle
        );
    assert(retval==pdPASS);

    gl_sequence_tasks_sem = xSemaphoreCreateBinary();
    assert(gl_sequence_tasks_sem != ((void*)0));

    vTaskStartScheduler();
}


void vAssertCalled(char const * const filename, int line_num) {
    uint32_t volatile ul = 0u;

    (void) filename;
    (void) line_num;
    taskENTER_CRITICAL();

    // Set ul to a non-zero value using the debugger to step out
    // of this function.
    while( ul == 0 ) {
        portNOP();
    }

    taskEXIT_CRITICAL();
}
