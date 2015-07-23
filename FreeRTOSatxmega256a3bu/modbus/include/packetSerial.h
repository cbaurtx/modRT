/* packetSerial.h
 *
 * Copyright (C) 2015 Christof Baur
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the license.MIT file for details.
 */

#ifndef PACKETSERIAL_H
#define PACKETSERIAL_H

// Baud rate 19200 for 32MhZ Xmega clock, this only works if 32Mhz clock is used and autocalibrated to good reference
// values from manual
#define PACKET_BAUDCTRLB    0x30
#define PACKET_BAUDCTRLA    0x0c

#define PACKET_TXDISABLEDELAY 0x5091

#define PACKET_RXGAPDELAY  0x1adb
#define PACKET_RXDONEDELAY 0x3eaa

#define MAXPACKETSIZE       256 // allows for space needed bz CRC16


// data types
struct packetTp {
// tx buffer. This buffer is modbus compatible (max packet size) and DMA compatible (static)
// 2st field has length of packet after reception. Does not include CRC16; 1 <= length <= 254 (modbus spec)
volatile unsigned char TX[MAXPACKETSIZE+1];   // first byte in buffer has length of packet -1
volatile unsigned char RX[MAXPACKETSIZE+1];   // first byte in buffer has length of packet -1

// packetRXerror  1   parity error
//                2   framing error (within byte or 1.5 char gap between bytes)
//                3   RX CRC error (modbus uses CRC16 IBM)
//                4   RX buffer overflow, RX packet size invalid
//
// packetTXerror  1   DMA error (Transmission error)
//                4   TX packet size invalid
volatile unsigned char TXerror;
volatile unsigned char RXerror;

volatile unsigned char RXdump; // to flush USART data register
volatile unsigned char* RXptr;
volatile unsigned int RXcount;
volatile unsigned char RXcomplete;
volatile unsigned char crcTableInd;
volatile unsigned char CRC16low;
volatile unsigned char CRC16high;
};

// function prototypes
void packetInit(void);
// caution!
// does not init DMA or INT controller, only channels

void sendPacket (unsigned char *packetData);
void clearTXbuffer(void);
void ToTXbuffer (unsigned char *data);
void appendToTXbuffer (unsigned char *dataToAppend);
void sendBuffer (void);
void receiveBuffer(void);

void errorInit(void);

void sendError(unsigned char);


/*  Global variables

// tx buffer. This buffer is modbus compatible (max packet size) and DMA compatible (static)
// 2st field has length of packet after reception. Does not include CRC16; 1 <= length <= 254 (modbuis spec)
volatile unsigned char packetTX[MAXPACKETSIZE+1];
volatile unsigned char packetRX[MAXPACKETSIZE+1];

// packetRXerror  1   parity error
//                2   framing error (within byte or 1.5 char gap between bytes)
//                3   RX CRC error (modbus uses CRC16 IBM)
//                4   RX buffer overflow, RX packet size invalid
//
// packetTXerror  1   DMA error (Transmission error)
//                4   TX packet size invalid
volatile unsigned char packetTXerror;
volatile unsigned char packetRXerror;
*/
#endif
