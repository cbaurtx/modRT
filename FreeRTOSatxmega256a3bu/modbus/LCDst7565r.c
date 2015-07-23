/* LCDst7565r.c
 *
 * Copyright (C) 2015 Christof Baur
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the license.MIT file for details.
 */

#include "LCDst7565r.h"
#include "font8x16.h"
#include <avr/io.h>


/*      XMEGA must be running at 32MHz!
 *
 *      LCD Size = 128 x 32
 *      SPI interface: USARTD0 (USART in SPI master mode)
 *
 *      Xmega   Xplained    st7565r
 *      PE4     BACKLIGHT   LED-
 *      PF3     nSPI_SS     CSB1
 *      PA3     nRESET      RST
 *      PD0     REG_SEL     A0
 *      PD1     SPI_SCK     SCL
 *      PD3     SPI_MOSI    SDA(Si)
 *
 */


// function prototypes
void sendInitCommand (unsigned char command);
void sendCommand(unsigned char cmd);
void sendData(unsigned char data);

void LCDst7576init() {

// Setting BSEL of USARTD0
USARTD0_BAUDCTRLA = 15;

// Init USARTD0
// Port A3, D0, D1, D3, E4, F3 as outputs and high
// This has to be done in gpioPortmap.h !

// USARTD0 cmode master SPI
USARTD0_CTRLC |= USART_CMODE_gm;

// SPI inverted XCK and trigger on rising edge; SPI MSB first
PORTD_PIN1CTRL = PORT_INVEN_bm;
USARTD0_CTRLC =  0xC3;
// USARTD0 SPI MSB first

// USARTD0 enable
USARTD0_CTRLB |= USART_TXEN_bm;

    // Reset the ST7565r.
    // Port PF3 low
    PORTF_OUTCLR = 0x08;
    // Port PA3 low, wait 1ms, Port PA3high
    PORTA_OUTCLR = 0x08;
    // RTOS not running, yet => use delay loop
    __builtin_avr_delay_cycles (CPU_FREQ / 1000);   // wait 1ms
    PORTA_OUTSET = 0x08;
    __builtin_avr_delay_cycles (CPU_FREQ / 200);    // wait 5ms
    // Port PF3 high
    PORTF_OUTSET = 0x08;
    // Port PE4 high (Backlight off)
    PORTE_OUTCLR = 0x10;

    // send init commands
    sendInitCommand(0xa0);     // ADC normal
    sendInitCommand(0xa6);     // Display normal
    sendInitCommand(0xc8);     // set reverse scan
    sendInitCommand(0xa2);     // LCD bias set to 1/9
    sendInitCommand(0x2f);     // power control set
    __builtin_avr_delay_cycles (CPU_FREQ / 200);    // wait 5ms
    sendInitCommand(0xf8);     // booster ratio
    sendInitCommand(0x00);     // ratio is 2x, 3x, 4x
    sendInitCommand(0x21);     // resitor ratio
    sendInitCommand(0x1f);     // set contrast
    sendInitCommand(0x40);     // display start line address = 0
    sendInitCommand(0xb0);     // page to page -
    sendInitCommand(0x10);     // col address upper 4 bits to 0
    sendInitCommand(0x00);     // col address lower 4 bits to 0
    sendInitCommand(0xaf);     // display on

    LCDst7565r.control = 0;
    LCDst7565r.posCursor.col = 0;
    LCDst7565r.posCursor.row = 0;
    LCDst7565r.characterToDisplay = 0;
}

void LCDst7576setCursorPosition(unsigned int colRow) {
    if ((colRow >> 8) > (DISPLAYWIDTH / FONTWIDTH))
        return;
    if ((colRow & 0xff) > (DISPLAYWIDTH / FONTWIDTH))
        return;

    LCDst7565r.posCursor.colRow = colRow;

    // set column address to col * FONTWIDTH
    sendCommand(0x10 + ((LCDst7565r.posCursor.col << FONTWIDTHEXP) >> 4));
    sendCommand(0x00 + ((LCDst7565r.posCursor.col << FONTWIDTHEXP) & 0x0f));
    // set page to row * 2
    sendCommand((unsigned char)(0xb0 + (LCDst7565r.posCursor.row << FONTHEIGHTEXP)));
    };

void LCDst7576control(unsigned int control) {
    // 00 => backlight off, 0x01 => backlight on, 0x02 clear screen
    if (control & 0x02) {
        LCDst7576clearDisplay();
        return;
        }
    LCDst7576backlight(control & 0x01);
    }

void LCDst7576printChar(int characterToPrint) {
    unsigned int colCount;
    unsigned int pageCount;

    if ((characterToPrint < MINCHAR) | (characterToPrint > MAXCHAR))
        return;

    for (pageCount = 0; pageCount < (FONTHEIGHT / 8); pageCount++) {
        // one character spans several pages so we must deal with setting the cursor position
        sendCommand(0xb0 + (pageCount + (LCDst7565r.posCursor.row << FONTHEIGHTEXP)));
        sendCommand(0x10 + (( LCDst7565r.posCursor.col * FONTWIDTH) >> 4));
        sendCommand(0x00 + (( LCDst7565r.posCursor.col * FONTWIDTH) & 0x0f));
        for (colCount = 0; colCount < FONTWIDTH; colCount++)
            {
            sendData(fixedFont[(((unsigned char)characterToPrint -MINCHAR) * FONTHEIGHT * FONTWIDTH / 8) + colCount + (pageCount * FONTWIDTH)]);
            };
        };
    sendCommand(0xb0 + (LCDst7565r.posCursor.row << FONTHEIGHTEXP));
    LCDst7565r.posCursor.col +=1;
    if ( LCDst7565r.posCursor.col >= (DISPLAYWIDTH / FONTWIDTH)) {      // set cursor back to left
        LCDst7565r.posCursor.col=0;
        sendCommand(0x10);
        sendCommand(0x00);
        if (LCDst7565r.posCursor.row < (DISPLAYHEIGHT / FONTHEIGHT-1)) {
            LCDst7565r.posCursor.row +=1;
            sendCommand(0xb0 + (LCDst7565r.posCursor.row << FONTHEIGHTEXP));
             }
        }
    };

void LCDst7576backlight(char onOff) {
    if (onOff)
        PORTE_OUTSET = 0x10;
    else
        PORTE_OUTCLR = 0x10;
    };

void LCDst7576clearDisplay(void) {
    int colCount;
    int pageCount;

    // cursor top left
    sendCommand(0x10);
    sendCommand(0x00);
    sendCommand(0xb0);

    for (pageCount = 0; pageCount < DISPLAYHEIGHT / 8; pageCount++) {
        sendCommand(0xb0 + pageCount);
        sendCommand(0x10);
        sendCommand(0x00);
        for (colCount = 0; colCount < DISPLAYWIDTH; colCount++)
            sendData(0x00);
        }
    // cursor top left
    sendCommand(0x10);
    sendCommand(0x00);
    sendCommand(0xb0);
    LCDst7565r.posCursor.col = 0;
    LCDst7565r.posCursor.row = 0;
    }

void sendInitCommand (unsigned char command) {
// uses delay loop use this as long as RTOS is not running
PORTD_OUTCLR = 0x01;        // select command mode
PORTF_OUTCLR = 0x08;        // SPI _CS
USARTD0_DATA = command;     // write to usart
// wait, SPI_CS
__builtin_avr_delay_cycles (CPU_FREQ / 100000);    // minimum wait 10us (longer if interrupted)
PORTF_OUTSET = 0x08;        // SPI _CS
PORTD_OUTSET = 0x01;        // select data mode
}

void sendCommand(unsigned char command) {
// uses delay loop (maybe changed later), use this only when RTOS is running
PORTD_OUTCLR = 0x01;        // select command mode
PORTF_OUTCLR = 0x08;        // SPI _CS
USARTD0_DATA = command;     // write to usart
// wait, SPI_CS
__builtin_avr_delay_cycles (CPU_FREQ / 100000);    // minimum wait 10us (longer if interrupted)
PORTF_OUTSET = 0x08;        // SPI _CS
PORTD_OUTSET = 0x01;        // select data mode
}

void sendData(unsigned char data) {
// uses delay loop (maybe changed later), use this only when RTOS is running
PORTD_OUTSET = 0x01;        // select command mode
PORTF_OUTCLR = 0x08;        // SPI _CS
USARTD0_DATA = data;     // write to usart
// wait, SPI_CS
__builtin_avr_delay_cycles (CPU_FREQ / 100000);    // minimum wait 10us (longer if interrupted)
PORTF_OUTSET = 0x08;        // SPI _CS
}
