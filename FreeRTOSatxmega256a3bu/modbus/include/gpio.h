/* gpio.h
 *
 * Copyright (C) 2015 Christof Baur
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the license.MIT file for details.
 */

#ifndef GPIO_H
#define GPIO_H

// public function prototypes
void gpioInit (void);
unsigned char checkGpioWriteAddress(unsigned int);
void gpioWriteBit (unsigned int, unsigned char);
void gpioWriteBits (unsigned int, unsigned char, unsigned long);

unsigned char checkGpioReadAddress(unsigned int);
unsigned char gpioReadBit (unsigned int);
unsigned long gpioReadBits (unsigned int, unsigned char);
#endif


