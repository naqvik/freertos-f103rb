/** gpio-drivers.h
 */

#ifndef GPIO_DRIVERS_H
#define GPIO_DRIVERS_H

#include <stdint.h>
#include <stdbool.h>
#include <stm32f10x.h>

typedef uint32_t volatile * const Reg32;
void gpio_config_pin(GPIO_TypeDef* base, uint32_t pin, uint32_t bits4);
void gpio_pin_onoff(GPIO_TypeDef* base, uint32_t pin, bool on);


#endif // GPIO_DRIVERS_H
