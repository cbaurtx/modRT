/* LCDst7565r.h
 *
 * Copyright (C) 2015 Christof Baur
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the license.MIT file for details.
 */

#ifndef LCDST7565_H
#define LCDST7565_H

#define CPU_FREQ 32000000   // CPU frequency this must match the actual frequency. Only 32Mhz for now
#define DISPLAYWIDTH    128
#define DISPLAYHEIGHT   32

// for global storage and modbus
struct {
    int control;                // bit0 backlight on/off; bit1 clear screen
    union {
        int colRow;             // low byte = col, high byte = row
        struct {
            unsigned char col;
            unsigned char row;
            }__attribute__((packed));
        }__attribute__((packed)) posCursor;
    int characterToDisplay;
    } LCDst7565r;

// function prototypes
void LCDst7576init(void);
void LCDst7576control(unsigned int control);
void LCDst7576setCursorPosition(unsigned int colRow);
void LCDst7576printChar(int characterToPrint);
void LCDst7576clearDisplay(void);
void LCDst7576backlight(char onOff);

#endif

