/* regsMap.h
 *
 * Copyright (C) 2015 Christof Baur
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the license.MIT file for details.
 */

#ifndef REGSMAP_H
#define REGSMAP_H

// all modules with functions which are listed in the table
#include "modbus.h"
#include "LCDst7565r.h"
#include "testReg.h"
#include "adc.h"

// note modbus addresses do not have to be continues. This allows for easy expansion
// Format: modbus address, C variable address, execute this after register was written by modbus,
// execute this before register is read by modbus
const regsMapType mapRegs[] = {
        {0x0000, &(modbus.slaveAddress), 0, pass, pass},

        {0x0080, &(testReg.a), 2, pass, pass},
        {0x0081, &(testReg.dummy), -1, pass, pass},
        {0x0083, &(testReg.b), -1, pass, pass},
        {0x0086, &(testReg.c), -1, pass, pass},
        {0x0087, &(testReg.e), -1, pass, pass},

        {0x0100, &(LCDst7565r.control), -1, LCDst7576control, pass},
        {0x0101, &(LCDst7565r.posCursor.colRow), -1, LCDst7576setCursorPosition, pass},
        {0x0102, &(LCDst7565r.characterToDisplay), -1, LCDst7576printChar, pass},

        {0x0180, &(adcSum8), -1, pass, pass},
    };


#define NR_OF_REGS 10
const int regsNrOfRegs = NR_OF_REGS;

#endif

