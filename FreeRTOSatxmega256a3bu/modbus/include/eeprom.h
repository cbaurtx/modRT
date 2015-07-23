/* eeprom.h
 *
 * Copyright (C) 2015 Christof Baur
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the license.MIT file for details.
 */

void eepromInit(void);
unsigned int eepromRead(unsigned int address);
void eepromWrite(unsigned int address, unsigned int data);
