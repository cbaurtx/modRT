/* packetSerial.c
 *
 * Copyright (C) 2015 Christof Baur
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the license.MIT file for details.
 */

#include "packetSerial.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "dma.h"

#include "FreeRTOS.h"
#include "semphr.h"


xSemaphoreHandle xTXsemaphore;
xSemaphoreHandle xRXsemaphore;
static volatile portBASE_TYPE xHigherPriorityTaskWoken;

struct packetTp packet;

void toTXbuffer (unsigned char *data);


/* Table of CRC values for high–order byte */
static unsigned char tableCRC16high[] = {
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
} ;

/* Table of CRC values for low–order byte */
static char tableCRC16low[] = {
0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2,
0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04,
0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E,
0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8,
0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6,
0xD2, 0x12, 0x13, 0xD3, 0x11, 0xD1, 0xD0, 0x10,
0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32,
0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE,
0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38,
0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA,
0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C,
0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0,
0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62,
0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4,
0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE,
0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA,
0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C,
0xB4, 0x74, 0x75, 0xB5, 0x77, 0xB7, 0xB6, 0x76,
0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54,
0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E,
0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98,
0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A,
0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86,
0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};


void packetInit(void){

    vSemaphoreCreateBinary( xTXsemaphore );
    vSemaphoreCreateBinary( xRXsemaphore );

    packet.RXptr = packet.RX;
    packet.RXcomplete = 0; // RX packet empty (not complete)
    packet.RXcount = 0;

    packet.TXerror=0;
    packet.RXerror=0;

    /* USART dma uses dma channel 1 (hardcoded)
       This way channels with higher priority (ch0) and lower priority
       ch2 and ch3 are left for other purposes
    */

    // channel disabled, no channel reset, repeat mode disabled, no transfer request.
    // as transfer is requested by USART, enable single shot mode, burst length is one byte
    // no interrupt on dma error, no transaction complete interrupt (to be used later)

    DMA.CH0.CTRLA = 0;
    DMA.CH0.CTRLA = 0x40;
    DMA.CH2.CTRLA = 0;
    DMA.CH2.CTRLA = 0x40;
    DMA.CH3.CTRLA = 0;
    DMA.CH3.CTRLA = 0x40;

    DMA.CH1.CTRLA = DMA_CH_SINGLE_bm | DMA_CH_BURSTLEN_1BYTE_gc;
    DMA.CH1.CTRLB = DMA_CH_TRNINTLVL1_bm;

    DMA.CH1.ADDRCTRL = DMA_CH_SRCRELOAD_BLOCK_gc        // reload source address at end of block
                           | DMA_CH_SRCDIR_INC_gc       // increment source address
                           | DMA_CH_DESTRELOAD_NONE_gc  // destination address is fixed
                           | DMA_CH_DESTDIR_FIXED_gc;

    DMA.CH1.TRIGSRC = DMA_CH_TRIGSRC_USARTC0_DRE_gc;   // USARTC0 to trigger dma (trigger when Usart data reg empty)

    // dma source. The 1st byte of the dma buffer has tge packet length and is not to be trasnmitted
    DMA.CH1.SRCADDR0 = (uint8_t)(( (uint32_t)packet.TX +1) & 0x00ff);        // lowest 8 bits of address (low byte)
    DMA.CH1.SRCADDR1 = (uint8_t)(( ( (uint32_t)packet.TX + 1)>> 8 ) & 0xff);    // niddle 8 bits
    DMA.CH1.SRCADDR2 = (uint8_t)(( ( (uint32_t)packet.TX + 1)>> 16 ) & 0xff);   // upper 8bits alwa

    // dma destination **** this does not work. Why ????
    // DMA.CH1.DESTADDR0 = (uint8_t)(((uint32_t)USARTC0_DATA) & 0xff);
    // DMA.CH1.DESTADDR1 = (uint8_t)((USARTC0_DATA >> 8) & 0xff);
    // DMA.CH1.DESTADDR2 = (uint8_t)(((uint16_t)USARTC0_DATA >> 16) & 0xff);

    DMA.CH1.DESTADDR0=0xA0;
    DMA.CH1.DESTADDR1=0x08;
    DMA.CH1.DESTADDR2=0x00;

    /* use USARTC0 */
    USARTC0_BAUDCTRLB = PACKET_BAUDCTRLB;
    USARTC0_BAUDCTRLA = PACKET_BAUDCTRLA;
    //Enable RX interrupt
    USARTC0_CTRLA = USART_RXCINTLVL_HI_gc;;

    //8 data bits, even parity, 1 stop bit
    USARTC0_CTRLC = USART_CHSIZE_8BIT_gc | USART_PMODE_EVEN_gc;

    // flush USART RX buffer
    while (USARTC0.STATUS & 0x80)       // there are RX data
        packet.RXdump = USARTC0.DATA;

    //Enable receive and transmit
    USARTC0_CTRLB = USART_TXEN_bm | USART_RXEN_bm;
    PORTC.DIRSET = PIN3_bm;                             // PC3 (TXD0) as output.


    PORTC.OUT &= 0xef;       // PC4 low, PC5 high (TX driver and RX receiver disable), PC6,7 high (LEDs off)
    PORTC.OUT |= 0xe0;
    PORTC.DIR |= 0xf0;       // set PC4 to PC7 as outputs

    /* TX disablle must be delayed by 0.5ms after DMA done ISR triggers*/
    TCC1.PER =      PACKET_TXDISABLEDELAY;
    TCC1.CTRLA =    TC_CLKSEL_OFF_gc;   // do not start counting
    TCC1.CTRLB =    TC_WGMODE_NORMAL_gc; // Normal operation
    TCC1.INTCTRLA = TC_OVFINTLVL_MED_gc;  //Medium level interrupt
    TCC1.INTFLAGS = 0x01; // clear interrupts

    /* TCD0 is for RX */
    TCD0.PER =      PACKET_RXDONEDELAY;
    TCD0.CTRLA =    TC_CLKSEL_OFF_gc;     // do not start counting, yet
    TCD0.CTRLB =    TC_WGMODE_NORMAL_gc;  // Normal operation
    TCD0.INTCTRLA = TC_OVFINTLVL_MED_gc;  // Medium level interrupt
    TCD0.INTFLAGS = 0x01; // clear interrupts

    // enable RX receiver
    PORTC.OUT &= 0xdf;          // PC5 low
   }

void sendPacket (unsigned char *packetData) {
    unsigned int packetLength;
    unsigned char *packetDataPtr;
    unsigned char *packetTXptr;
    unsigned int count;

    packet.TXerror = 0;

    // modbus payload size is min 1 byte and max 254 bytes
    if (*packetData == 0)
        packet.TXerror |= 0x08;

    else {

        if (*packetData > 254) {
            packet.TXerror |= 0x08;
            packetLength=254;   // truncate and send max possible
            }
        else
            packetLength = *packetData;

        packetDataPtr = packetData;
        packetTXptr = packet.TX;

        packet.CRC16low = 0xff;
        packet.CRC16high = 0xff;

        // copy packet data to DMA TX buffer

        // packet length field is not included in CRC
        packetTXptr++;
        packetDataPtr++;

        for (count = packetLength; count > 0; ) {
            *packetTXptr = *packetDataPtr;

            packet.crcTableInd = *packetTXptr ^ packet.CRC16low;
            packet.CRC16low = packet.CRC16high ^ tableCRC16high[packet.crcTableInd];
            packet.CRC16high = tableCRC16low[packet.crcTableInd];

            packetTXptr++;
            packetDataPtr++;
            count--;
            }
        // add CRC 16-IBM to packet
        *packetTXptr = packet.CRC16low;
        packetTXptr++;
        packetLength++;
        *packetTXptr = packet.CRC16high;
        packetLength++;

        // set TCC1 to delay 4.5 * 11 / baudrate.
        // 4.5 = 1 (for sending char just transfered to USART) + 3.5 (modbus spec)
        TCC1.PER = PACKET_TXDISABLEDELAY;

        // set DMA block length
        DMA.CH1.TRFCNT = packetLength;

        // disable RX receiver so we do not RX data we are sending
        PORTC.OUT |= 0x20;     // PC5 high

        // enable TX driver
        PORTC.OUT |= 0x10;     // PC4 high

        // enable the DMA channel.
        DMA.CH1.CTRLA |=  DMA_CH_ENABLE_bm;

        xSemaphoreTake( xTXsemaphore, portMAX_DELAY);
        }
}


void toTXbuffer (unsigned char *data) {
    // 1st byte has length
    unsigned char *packetTXptr;
    unsigned char *packetDataPtr;

    unsigned int count;

    packetDataPtr = data;
    count = *packetDataPtr;
    packetDataPtr++;    // skip 1st byte containing length

    packetTXptr = packet.TX;
    packetTXptr++;

    while (count > 0) {
        *packetTXptr = *packetDataPtr;
        packetDataPtr++;
        packetTXptr++;
        count--;
        }
    *packet.TX = *data; // update TX buffer length
}

void appendToTXbuffer (unsigned char *dataToAppend) {
    // 1st byte has length
    unsigned char *packetTXptr;
    unsigned char *packetDataPtr;

    unsigned int count;

    packetDataPtr = dataToAppend;
    count = *packetDataPtr;
    packetDataPtr++;    // skip 1st byte containing length

    packetTXptr = packet.TX + *packet.TX +1;  // set pointer to end of buffer data (*packet.tx has length)

    while (count > 0) {
        *packetTXptr = *packetDataPtr;
        packetTXptr++;
        packetDataPtr++;
        count--;
        }
    *packet.TX += *dataToAppend; // update TX buffer length
}

void sendBuffer (void) {
    unsigned int packetLength;
    unsigned char *packetTXptr;
    unsigned int count;

    packet.TXerror = 0;

    // modbus payload size is min 1 byte and max 254 bytes
    if (*packet.TX == 0)
        packet.TXerror |= 0x08;

    else {

        if (*packet.TX > 254) {
            packet.TXerror |= 0x08;
            packetLength=254;   // truncate and send max possible
            }
        else
            packetLength = *packet.TX;

        packetTXptr = packet.TX;
        packetTXptr++;         // skip packet size field
        packet.CRC16low = 0xff;
        packet.CRC16high = 0xff;

        for (count = packetLength; count > 0; ) {
            packet.crcTableInd = *packetTXptr ^ packet.CRC16low;
            packet.CRC16low = packet.CRC16high ^ tableCRC16high[packet.crcTableInd];
            packet.CRC16high = tableCRC16low[packet.crcTableInd];

            packetTXptr++;
            count--;
            }

        // add CRC 16-IBM to packet
        *packetTXptr = packet.CRC16low;
        packetTXptr++;
        packetLength++;
        *packetTXptr = packet.CRC16high;
        packetLength++;

        // set TCC1 to delay 4.5 * 11 / baudrate.
        // 4.5 = 1 (for sending char just transfered to USART) + 3.5 (modbus spec)
        TCC1.PER = PACKET_TXDISABLEDELAY;

        // set DMA block length
        DMA.CH1.TRFCNT = packetLength;

        // disable RX receiver
        PORTC.OUT |= 0x20;     // PC5 high

        // enable TX driver
        PORTC.OUT |= 0x10;     // PC4 high

        // enable the DMA channel.
        DMA.CH1.CTRLA |=  DMA_CH_ENABLE_bm;

        xSemaphoreTake( xTXsemaphore, portMAX_DELAY);
        }
}

void receiveBuffer(void) {
    // packetRXerror = 0;
    packet.RXptr = packet.RX;
    packet.RXptr++;
    packet.RXcount = 0;
    packet.RXcomplete = 0;       // RX packet empty (not complete)

    xSemaphoreTake( xRXsemaphore, portMAX_DELAY);

    if (packet.RXcount > 0x00ff)
        *packet.RX = 254;        // do not include CRC in size checking
    else
        packet.RXcount--;
        packet.RXcount--;
        *packet.RX = (uint8_t)packet.RXcount;


    if ((packet.CRC16high != 0) | (packet.CRC16low != 0))
        packet.RXerror |= 0x04;

    if (packet.RXerror == 0)
        PORTC.OUT |= 0x40;      // error LED off
    else
        PORTC.OUT &= 0xbf;      // error LED on


}

ISR(DMA_CH1_vect){

TCC1.CTRLFSET =    TC_CMD_RESTART_gc;  // set TCC1 back to 0
TCC1.CTRLA =   TC_CLKSEL_DIV4_gc;      // prescaler 4, use system clock, this enables counting
if (DMA.INTFLAGS & DMA_CH1TRNIF_bm)
    {
        DMA.INTFLAGS = DMA_CH1TRNIF_bm;
        packet.TXerror = 0;
        PORTC.OUT |= 0x40;          // error LED off
    }
if (DMA.INTFLAGS & DMA_CH1ERRIF_bm)
    {
        DMA.INTFLAGS = DMA_CH1ERRIF_bm;
        packet.TXerror=1;
        PORTC.OUT &= 0xbf;       // error LED on
    }

}






ISR (USARTC0_RXC_vect) {
    if (packet.RXcount == 0) {
        // 1st byte received, packet starts; enable timer TCD0
        TCD0.CTRLFSET =    TC_CMD_RESTART_gc;      // set TCD0 back to 0
        TCD0.CTRLA =       TC_CLKSEL_DIV4_gc;      // prescaler 4, use system clock, this enables counting
        PORTC.OUT &= 0x7f;                         // modbus receive LED on

        if (USARTC0.STATUS & (USART_PERR_bm | USART_BUFOVF_bm))   {    // parity error, overflow
            packet.RXerror = 0x01;
            PORTC.OUT &= 0xbf; // modbus error LED on
            }
        else          // no RX errors
            packet.RXerror = 0x00;

        *packet.RXptr = USARTC0.DATA;
        //packetCRC16 =    0xff ^ CRC16TABLE[(packetCRC16 ^ *packetRXptr) & 0xff];
        packet.crcTableInd = ~(*packet.RXptr);
        packet.CRC16low =  ~tableCRC16high[packet.crcTableInd];
        packet.CRC16high = tableCRC16low[packet.crcTableInd];

        packet.RXptr++;
        packet.RXcount++;
        }
    else {
        if ((TCD0.CNT > PACKET_RXGAPDELAY) && (! packet.RXcomplete))  // frame error
            packet.RXerror |= 0x02;

        TCD0.CTRLFSET =    TC_CMD_RESTART_gc;   // set TCD0 back to 0
        if ((packet.RXcount < MAXPACKETSIZE) && (! packet.RXcomplete)) {
            if (USARTC0.STATUS & (USART_PERR_bm | USART_BUFOVF_bm))   {    // parity error, overflow
                packet.RXerror = 0x01;
                PORTC.OUT &= 0xbf; // modbus error LED on
                }
            *packet.RXptr = USARTC0.DATA;

            packet.crcTableInd = *packet.RXptr ^ packet.CRC16low;
            packet.CRC16low = packet.CRC16high ^ tableCRC16high[packet.crcTableInd];
            packet.CRC16high = tableCRC16low[packet.crcTableInd];
            packet.RXptr++;
            packet.RXcount++;
            }
        else {
            packet.RXdump = USARTC0.DATA;    // to avoid locking
            packet.RXerror |= 0x08;
            }
        }
    }

ISR (TCC1_OVF_vect) {
    // disable TX driver
    PORTC.OUT &= 0xef;     // PC4 low

    // enable RX receiver
    PORTC.OUT &= 0xdf;     // PC5 low


    TCC1.CTRLA =    TC_CLKSEL_OFF_gc;   // disable counting
    xSemaphoreGiveFromISR( xTXsemaphore, &xHigherPriorityTaskWoken );
    TCC1.INTFLAGS = 0x01;  // clear interrupts
    }

ISR (TCD0_OVF_vect) {
    packet.RXcomplete = 1;
    TCD0.CTRLA =    TC_CLKSEL_OFF_gc;     // disable TCD0
    PORTC.OUT |= 0x80;                    // modbus receive LED off

    // enable RX receiver
    PORTC.OUT &= 0xdf;          // PC5 low
    xSemaphoreGiveFromISR( xRXsemaphore, &xHigherPriorityTaskWoken );
    }

void errorInit(void){
    /* use USARTE0 */
    USARTE0_BAUDCTRLB = PACKET_BAUDCTRLB;
    USARTE0_BAUDCTRLA = PACKET_BAUDCTRLA;
    //Disable interrupts
    USARTE0_CTRLA = 0;
    //8 data bits, no parity and 1 stop bit
    USARTE0_CTRLC = USART_CHSIZE_8BIT_gc;
    //Enable receive and transmit
    USARTE0_CTRLB = USART_TXEN_bm | USART_RXEN_bm; // And enable high speed mode
    PORTE.DIRSET = PIN3_bm; // PC3 (TXD0) as output.
    }


void sendError(unsigned char errorNo) {
    char errorMessage[10] = "Error ??\n";
    char *errorMessagePtr;

    errorMessage[7] = ((errorNo & 0x0f) < 0x0a) ? (errorNo & 0x0f) + '0':  (errorNo & 0x0f) + 'a';
    errorMessage[6] = ((errorNo >> 4) < 0x0a) ? (errorNo >> 0x04) + '0':  (errorNo >> 4) + 'a';

    errorMessagePtr = errorMessage;
    while (*errorMessagePtr) {
        while( !(USARTE0_STATUS & USART_DREIF_bm) );
        USARTE0_DATA = *errorMessagePtr;
        errorMessagePtr++;
    }
}

