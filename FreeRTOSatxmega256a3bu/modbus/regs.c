/* regs.c
 *
 * Copyright (C) 2015 Christof Baur
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the license.MIT file for details.
 */

#include "regs.h"

typedef struct regsMapField   {
  int address;                                  // modbus address
  int *variable;
  int eepromAddress;                            // EEPROM address 0 is reserved for modbus slave address
                                                // negative value: do not use EEPROM
  void (* const ptrRegWrite)(unsigned int);     // executed after register was written by modbus
  void (* const ptrRegRead)(unsigned int);      // executed before register is read by modbus
} regsMapType;

#include "regsMap.h"


int regsSearchAddress(int address) {
    int low;
    int high;
    int middle;

    low = 0;
    high = NR_OF_REGS - 1;

    while (low <= high) {
        middle = low + (high - low)/2;
        if (address < mapRegs[middle].address)
            high = middle -1;
        else if (address > mapRegs[middle].address)
                low = middle  + 1;
             else
                return middle;
        }
    return -1; // address not in map table
}

void regsInit(void) {
    int regsCount;

    // load shadowed register from EEPROM
    for (regsCount=0; regsCount < NR_OF_REGS; regsCount++) {
        if (mapRegs[regsCount].eepromAddress >= 0)
            *(mapRegs[regsCount].variable) = eepromRead(mapRegs[regsCount].eepromAddress);
        };
    };


int regsReadSingle(int mappedAddress){
    // execute before read function
    mapRegs[mappedAddress].ptrRegRead(mappedAddress);
    return *(mapRegs[mappedAddress].variable);
}

void regsWriteSingle(int mappedAddress, int data) {
    *(mapRegs[mappedAddress].variable) = data;
    if (mapRegs[mappedAddress].eepromAddress >= 0) {
        eepromWrite(mapRegs[mappedAddress].eepromAddress, data);
        }


    // execute after write funtion
    mapRegs[mappedAddress].ptrRegWrite(data);
}

void pass(int dummy) {
    // gpioWriteBit(18,!gpioReadBit(18));
}





