/* regs.h
 *
 * Copyright (C) 2015 Christof Baur
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the license.MIT file for details.
 */

#ifndef REGS_H
#define REGS_H

void regsInit(void);
int regsSearchAddress(int address);
int regsReadSingle(int mappedAddress);
void regsWriteSingle(int mappedAddress, int data);

void ptrRegWrite (unsigned int);
void ptrRegRead (unsigned int);
void pass(int dummy);

#endif

