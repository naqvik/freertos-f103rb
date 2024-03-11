/**
   Serial I/O.  For Keil we can just implement fputc and fgetc
 */
#ifndef SERIAL_IO_H
#define SERIAL_IO_H

#include <stdio.h>

int fputc(int c, FILE *stream);
int fgetc(FILE *stream);
void openUsart2();

#endif //SERIAL_IO_H
