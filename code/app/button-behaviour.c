/** -*- c++ -*-
   Implement button-press behaviour

   The button will increment a global counter, this will be read
   by the displayPattern thread, and will alter the output.
 */

#include <assert.h>
#include "gpio-drivers.h"
#include "bsp.h"

// disable warning about no prototype, can solve by including
// widget.h, then the following line can be removed
void configureButton(void);


void configureButton(void) {
    // The blue USER button on the nucleo-f103rb is on PC13.

    // Turn on GPIOC clock
    enable_gpioc_clk();

    // Turn on AFIO clock
    enable_afio_clk();
    
    // Pin PC13 should be configured as input, floating.
    // This corresponds to binary 0100, or 0x4

    gpio_config_pin(GPIOC, Pin13, 0x4);

    // enable trigger on falling edge
    exti_falling_edge_trig(Pin13, true);

    // select the interrupt source to be pin 13 of port C
    // that is AFIO_EXTICR[3] nybble 1 must be set to 0x2

    //AFIO->EXTICR[3] |= (0x2u << 4); // bits[7:4] <- 0010
    afio_exticr_source(PortC, Pin13);

    // Since this is an external device (on a pin13), we'll need to
    // unmask interrupt EXTI 13.
    exti_unmask(Pin13, true);

    // enable the associated interrupt on the NVIC.  EXTI 13 falls in
    // the range EXTI 15-10, all of which are mapped to the IRQ 40
    // EXTI15_10 (IRQ40 aka exception 56)

    // kjn
    // NVIC_set_enable(40);
    // CMSIS
    NVIC_EnableIRQ(40);

    NVIC_SetPriority(40, 20);
}
