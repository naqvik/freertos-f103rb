/**
   bsp.h: Interface to board Support Package for nucleo-f103rb
*/
#ifndef BSP_H
#define BSP_H

#include <stdint.h>
void NVIC_set_enable(uint32_t irq_num);

#endif // BSP_H
