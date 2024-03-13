/** Blinks the LEDS
 */
#include <stdio.h>
#include <ctype.h>
#include <stm32f10x.h>

// FIXME: I think all freertos stuff should be in a single file
#include "FreeRTOS.h"
#include "task.h"
#include "widget.h"
#include "gpio-drivers.h"

void configureWidget() {
    // turn on clock for GPIOA and GPIOB
    RCC->APB2ENR |= 1u<<2;
    RCC->APB2ENR |= 1u<<3;

    // configure four pins to be output, push-pull, 50MHz
    // PB10: Red, PA8: Yellow, PA9: Green, PB6: Blue
    gpio_config_pin(GPIOB, 10u, 3u);
    gpio_config_pin(GPIOA, 8u, 3u);
    gpio_config_pin(GPIOA, 9u, 3u);
    gpio_config_pin(GPIOB, 6u, 3u);

}

void runWidget() {
    static const struct {
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

    printf("Press any key to initiate one cycle: ");
    int c = fgetc(stdin);
    if (isprint(c))
        printf("\nKey pressed: %c (0x%02x)\n", c, c);
    else
        printf("\nNon-printable key pressed: 0x%02x\n", c);
    for (uint32_t i=0; i < 8; ++i) {
        GPIO_TypeDef* gpio = seq[i].gpio;
        uint32_t pin = seq[i].pin;
        uint32_t onoff = seq[i].onoff;

        gpio_pin_onoff(gpio, pin, onoff);
        vTaskDelay(500);
    }
}
