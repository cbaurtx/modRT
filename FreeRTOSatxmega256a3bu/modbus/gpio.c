/* gpio.c
 *
 * Copyright (C) 2015 Christof Baur
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the license.MIT file for details.
 */

#include <avr/io.h>
#include "gpio.h"

// private function prototypes
void writePortA(unsigned char, unsigned char);
void writePortB(unsigned char, unsigned char);
void writePortC(unsigned char, unsigned char);
void writePortE(unsigned char, unsigned char);
void writePortF(unsigned char, unsigned char);
void writePortR(unsigned char, unsigned char);
unsigned char readPortA(unsigned char);
unsigned char readPortB(unsigned char);
unsigned char readPortC(unsigned char);
unsigned char readPortE(unsigned char);
unsigned char readPortF(unsigned char);
unsigned char readPortR(unsigned char);

typedef struct portWriteMapField   {
  void (* const ptrDoIO)(unsigned char, unsigned char);
  unsigned char IOsubAddress;
} portWriteMapType;

typedef struct portReadMapField   {
  unsigned char (* const ptrDoIO)(unsigned char);
  unsigned char IOsubAddress;
} portReadMapType;


#include "gpioPortMap.h"      // do not move this!

/*
 During init port direction is set. Writing to a port set as input does no harm.
 Therfore we leave holes in the modbus address space, which is similar to not connecting a wire to an output.
 This method seams to be not as bad as subsequent IO bits shifting when an IO pin is changed from an
 output to an input or used by special peripherals.
*/

void gpioInit (void) {

    int count;
    static const unsigned char ioctrl_porta[] = IOCTRL_PORTA;
    static const unsigned char ioctrl_portb[] = IOCTRL_PORTB;
    static const unsigned char ioctrl_portc[] = IOCTRL_PORTC;
    static const unsigned char ioctrl_portd[] = IOCTRL_PORTD;
    static const unsigned char ioctrl_porte[] = IOCTRL_PORTE;
    static const unsigned char ioctrl_portf[] = IOCTRL_PORTF;
    static const unsigned char ioctrl_portr[] = IOCTRL_PORTR;

    // set IO controls
    for (count = 0; count < 8; ) {
        PORTCFG.MPCMASK = (0x01 << count);
        PORTA.PIN0CTRL = ioctrl_porta[count];
        count++;
    }
    for (count = 0; count < 8; ) {
        PORTCFG.MPCMASK = (0x01 << count);
        PORTB.PIN0CTRL = ioctrl_portb[count];
        count++;
    }
    for (count = 0; count < 8; ) {
        PORTCFG.MPCMASK = (0x01 << count);
        PORTC.PIN0CTRL = ioctrl_portc[count];
        count++;
    }
    for (count = 0; count < 8; ) {
        PORTCFG.MPCMASK = (0x01 << count);
        PORTD.PIN0CTRL = ioctrl_portd[count];
        count++;
    }
    for (count = 0; count < 8; ) {
        PORTCFG.MPCMASK = (0x01 << count);
        PORTE.PIN0CTRL = ioctrl_porte[count];
        count++;
    }
    for (count = 0; count < 8; ) {
        PORTCFG.MPCMASK = (0x01 << count);
        PORTF.PIN0CTRL = ioctrl_portf[count];
        count++;
    }
    for (count = 0; count < 8; ) {
        PORTCFG.MPCMASK = (0x01 << count);
        PORTR.PIN0CTRL = ioctrl_portr[count];
        count++;
    }

    // set port direction
    PORTA.DIR = DIR_PORTA;
    PORTB.DIR = DIR_PORTB;
    PORTC.DIR = DIR_PORTC;
    PORTD.DIR = DIR_PORTD;
    PORTE.DIR = DIR_PORTE;
    PORTF.DIR = DIR_PORTF;
    PORTR.DIR = DIR_PORTR;
}

void writePortA(unsigned char subaddress, unsigned char value) {
    if (value)
        PORTA_OUTSET = subaddress;
    else
        PORTA_OUTCLR = subaddress;
}

void writePortB(unsigned char subaddress, unsigned char value) {
    if (value)
        PORTB_OUTSET = subaddress;
    else
        PORTB_OUTCLR = subaddress;
}

void writePortC(unsigned char subaddress, unsigned char value) {
    if (value)
        PORTC_OUTSET = subaddress;
    else
        PORTC_OUTCLR = subaddress;
}

void writePortE(unsigned char subaddress, unsigned char value) {
    if (value)
        PORTE_OUTSET = subaddress;
    else
        PORTE_OUTCLR = subaddress;
}


void writePortF(unsigned char subaddress, unsigned char value) {
    if (value)
        PORTF_OUTSET = subaddress;
    else
        PORTF_OUTCLR = subaddress;
}


void writePortR(unsigned char subaddress, unsigned char value) {
    if (value)
        PORTR_OUTSET = subaddress;
    else
        PORTR_OUTCLR = subaddress;
}

unsigned char readPortA(unsigned char subaddress) {
    if (PORTA_IN & subaddress)
        return 1;
    else
        return 0;
}
unsigned char readPortB(unsigned char subaddress) {
    if (PORTB_IN & subaddress)
        return 1;
    else
        return 0;
}
unsigned char readPortC(unsigned char subaddress) {
    if (PORTC_IN & subaddress)
        return 1;
    else
        return 0;
}
unsigned char readPortD(unsigned char subaddress) {
    if (PORTD_IN & subaddress)
        return 1;
    else
        return 0;
}
unsigned char readPortE(unsigned char subaddress) {
    if (PORTE_IN & subaddress)
        return 1;
    else
        return 0;
}
unsigned char readPortF(unsigned char subaddress) {
    if (PORTF_IN & subaddress)
        return 1;
    else
        return 0;
}
unsigned char readPortR(unsigned char subaddress) {
    if (PORTR_IN & subaddress)
        return 1;
    else
        return 0;
}

unsigned char checkGpioWriteAddress(unsigned int address) {
    return (address <= MAX_OUT_ADDRESS);         // unsigned int cannot be smaller than 0
}

unsigned char checkGpioReadAddress(unsigned int address) {
    return (address <= MAX_IN_ADDRESS);
}


void gpioWriteBit(unsigned int portAddress, unsigned char value) {
   mapWriteIO[portAddress].ptrDoIO(mapWriteIO[portAddress].IOsubAddress, value);
}

void gpioWriteBits(unsigned int portAddress, unsigned char nrOfBits, unsigned long Values) {
    unsigned int addressToWriteTo;
    unsigned char valueToWrite;

    valueToWrite = Values;
    for (addressToWriteTo = portAddress; addressToWriteTo < portAddress + nrOfBits; addressToWriteTo++ ) {
        mapWriteIO[addressToWriteTo].ptrDoIO(mapWriteIO[addressToWriteTo].IOsubAddress, valueToWrite &0x01);
        valueToWrite >>= 1;
        }
}

unsigned char gpioReadBit(unsigned int portAddress) {
    return (mapReadIO[portAddress].ptrDoIO(mapReadIO[portAddress].IOsubAddress));
}


unsigned long gpioReadBits(unsigned int portAddress, unsigned char nrOfBits) {  // more than 32 bits not supported
    unsigned int bitAddress;
    unsigned long values;

    values = 0;
    for (bitAddress = portAddress + nrOfBits -1; bitAddress >= portAddress; bitAddress--) {     // for nrOfBits == 1 read portAddress
        values <<= 1;
        values |= mapReadIO[bitAddress].ptrDoIO(mapReadIO[bitAddress].IOsubAddress);
        }

    return values;
}
