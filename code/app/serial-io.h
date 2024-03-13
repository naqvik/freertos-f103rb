/**
   Serial I/O.  For Keil we can just implement fputc and fgetc (in serial-io.c)
   and clients will get the prototypes by including stdio.h.

   Note: this implementation depends on MicroLib being enabled (which means no C++).
 */
#ifndef SERIAL_IO_H
#define SERIAL_IO_H

#include <stdio.h>

void openUsart2(void);

#endif //SERIAL_IO_H
