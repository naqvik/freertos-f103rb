// -*- c++ -*-
/**
   Adapted from M. Samek's Embedded Systems Progamming video course,
   lesson 13-15 Startup Code, 16-18 Interrupts.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>     // for abort() used by __aeabi_assert()
#include <stm32f10x.h>

// prototypes
#include "serial-io.h"
static void sendByte (char c);
static char getByte (void);

__attribute__((noreturn))
/** Implement standard ARM assert function.

    This function is not already linked in because I'm using microlib, which
    does not provide it.
 */
void __aeabi_assert(char const *expr, char const * filename, int line);

// implement basic functions needed to retarget std C I/O
static void sendByte (char c)
{
    if (c == '\n') {  // insert CR before each NL
        while ((USART2->SR & 1u<<7)==0) {  // spin while TDR is occupied
        }
        USART2->DR = '\r';
    }

    while ((USART2->SR & 1u<<7)==0) {  // spin while TDR is occupied
    }
    USART2->DR = c;
}

static char getByte (void)
{
    // while the read data register is empty, spin
    while ((USART2->SR & 1u<<5)==0) {
    }
    return (char)USART2->DR;
}

// Implement minimal functions required to retarget under Keil's microlib

int fgetc(FILE * stream) {
    (void)stream;
    char byte = getByte();
    return (int)byte;
}
int fputc(int c, FILE * stream) {
    (void)stream; // disable warning about unused parameter
    char byte = (char)c;  // avoid implicit conversion warning
    sendByte(byte);
    return c;
}

/** This function is to be called exactly once, generally at the start of main.

    This is the only public function defined here, and the only one
    present in the associated header file serial-io.h.

    Note: this implementation assumes the use of Keil's Microlib
 */
void openUsart2(void)
{
    // We are using USART2, which is the virtual com port on the f103rb
    // and which uses (by default) PA2 (Tx) and PA3 (Rx).
    RCC->APB2ENR |= 1u<<2; // bits[2]=IOPAEN=1, Enable clock for GPIOA
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    RCC->APB1ENR |= 1u<<17; // bits[17]=usart2en=1, enable usart2

    // Clear the registers before setting them
    USART2->CR1 = 0x00;
    USART2->CR2 = 0x00;
    USART2->CR3 = 0x00;

    GPIOA->CRL &= ~(0xFFUL << 8); // clear PA2 and PA3
    GPIOA->CRL |= (0x0BUL << 8); // Sets PA2 as Alternate function output, push pull.
    GPIOA->CRL |= (0x04UL << 12); // Sets PA3 as a floating input (also the reset state).
    // Reference: RM0008 27.3.4 Fractional Baudrate Generation
    // Formula: BRR value = 36e6/bps (half-the-master-clk / bps)
    // Example: for 9600bps, BRR=36e6/9600 = 3750 = 0xEA6
    // Example: for 57,600bps, BRR=36e6/57600 = 625 = 0x271
    // Example: for 115.2kbps, BRR=36e6/115.2k = 312.5 = 312 = 0x138
    USART2->BRR = 312;

    /** configure USART2_CR1 */
    USART2->CR1 |= (1u << 13); // sets the USART enable (UE) bit

    // Sets the RE & TE bits for enabling the receiver/transmitter.
    USART2->CR1 |=
          0u<<0             // bits[0], SBK=0, don't send break
        | 0u<<1             // bits[1], RWU=0, receiver in active mode
        | 1u<<2             // bits[2], RE=1, enable receiver
        | 1u<<3             // bits[3], TE=1, enable transmitter
        | 0u<<4             // bits[4], IDLEIE=0, disable idle interrupt
        | 0u<<5             // bits[5], RXNEIE=0, disable RXNE interrupt
        | 0u<<6             // bits[6], RCIE=0, disable TC interrupt
        | 0u<<7             // bits[7], TXEIE=0, disable TXE interrupt
        | 0u<<8             // bits[8], PEIE=0, disable PE interrupt
        | 0u<<9             // bits[9], PS=0, select odd parity
        | 0u<<10            // bits[10], PCE=0, disable parity control
        | 0u<<11            // bits[11], WAKE=0, idle line
        | 0u<<12            // bits[12], M=0, set 0-8-n
        | 1u<<13            // bits[13], UE=1, usart enable
        | 0u<<14;           // bits[15:14]=00, reserved
}

__attribute__((noreturn))
void __aeabi_assert(char const *expr, char const * filename, int line) {
    fprintf(stderr, "** assertion failed: \"%s\", file: \"%s\":%d",
            expr, filename, line);

    abort();
}
