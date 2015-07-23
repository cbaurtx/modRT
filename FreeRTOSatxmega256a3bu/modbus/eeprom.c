/* eeprom.c
 *
 * Copyright (C) 2015 Christof Baur
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the license.MIT file for details.
 */
#include <avr/io.h>
#include "eeprom.h"

void eepromInit(void) {
    NVM.CTRLB = NVM_EEMAPEN_bm;                    // map EEPROM (no IO access)
    NVM.CMD = NVM_CMD_ERASE_EEPROM_BUFFER_gc;      // flush EEPROM page buffer
    while (NVM.STATUS & NVM_NVMBUSY_bm);           // wait for NVM operation to finish
    };


unsigned int eepromRead(unsigned int address) {
    while (NVM.STATUS & NVM_NVMBUSY_bm);           // wait for NVM operation to finish
    return(*(unsigned int *) (address + MAPPED_EEPROM_START));
    };


void eepromWrite(unsigned int address, unsigned int data) {
    unsigned int dataRead;
    while (NVM.STATUS & NVM_NVMBUSY_bm);           // wait for NVM operation to finish
    dataRead = *(unsigned int *) (address + MAPPED_EEPROM_START);

    if (dataRead == data)
        return;
    while (NVM.STATUS & NVM_NVMBUSY_bm);           // wait for NVM operation to finish
    *(unsigned int *) (address + MAPPED_EEPROM_START) = data;

    if ((~dataRead) &  data)                 // => atomic write
        NVM.CMD = NVM_CMD_ERASE_WRITE_EEPROM_PAGE_gc;
    else                                            //  ereasing not needed
        NVM.CMD = NVM_CMD_WRITE_EEPROM_PAGE_gc;
    asm volatile(
        "movw r30,  %0" "\n\t"
        "ldi  r16,  %2" "\n\t"
        "out  %3, r16" "\n\t"
        "st     Z,  %1"
        :
        : "r" (&NVM.CTRLA), "r" (NVM_CMDEX_bm), "M" (CCP_IOREG_gc), "m" (CCP)
        : "r16", "r30", "r31"
        );
    };

