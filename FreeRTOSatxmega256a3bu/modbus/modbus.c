/* modbus.c
 *
 * Copyright (C) 2015 Christof Baur
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the license.MIT file for details.
 */

#include "packetSerial.h"
// #include <avr/io.h>
#include "eeprom.h"
#include "modbus.h"
#include "FreeRTOS.h"       // also includes avr/io.h
#include "task.h"
#include "gpio.h"
#include "regs.h"

#define  ILLEGALFUNCTION      0x01
#define  ILLEGALDATAADDRESS   0x02
#define  ILLEGALDATAVALUE     0x03
#define  SERVERDEVICEFAILURE  0x04
#define  ACKNOWLEDGE          0x05
#define  SERVERDEVICEBUSY     0x06
#define  MEMORYPARITYERROR    0x08

// function prototypes
static void vModbus ( void *pvParameters );
void modbusException (unsigned char);
void checkAndProcessRequest(void);

void modbusInvalid(void);
void modbusWriteSingleCoil(void);
void modbusReadCoils(void);
void modbusWriteCoils(void);
void modbusReadDiscreateInputs(void);

void modbusReadHoldingRegisters (void);
void modbusWriteSingleRegister (void);
void modbusWriteHoldingRegisters (void);

void modbusDiagnostic (void);

void modbusDeviceID(void);

// function prototypes diagnostics
void diagnosticInvalid(void);
void diagnosticReturnQuery (void);
void diagnosticRestartCommunication (void);
void diagnosticRegister (void);
void diagnosticForceListenOnly (void);
void diagnosticClear (void);
void diagnosticBusMessageCount (void);
void diagnosticBusCommErrorCount (void);
void diagnosticBusExceptionCount (void);
void diagnosticSlaveMessageCount (void);
void diagnosticNoResponseCount (void);
void diagnosticNAK_Count (void);
void diagnosticSlaveOverrunCount (void);
void diagnosticClearSlaveOverrunCount (void);


struct {
  unsigned int busMessages;             // CPT1 no. of messages detected on bus. Messages with CRC error do not count
  unsigned int busCommunicationErrors;  // CPT2 CRC error, overrun, parity error, less than 3 bytes received
  unsigned int busExceptionErrors;      // CPT3 slave exceptions, including not returned exceptions (broadcast)
  unsigned int slaveMessages;           // CPT4 bo of messages addressed to this slaves. Includes broadcasted messages
  unsigned int slaveNoResponse;         // CPT5 no of received broadcast messages
  unsigned int slaveNAKs;               // CPT6 like busExceptionErrors but only returned (NAK) messages
  unsigned int slaveOverruns;           // CPT8
 } modbusCounter;

// task handle
xTaskHandle hModbus;

static void (*const modbusFuncmap[])(void) =
{
 modbusInvalid,                 // 0x00
 modbusReadCoils,               // 0x01
 modbusReadDiscreateInputs,     // 0x02
 modbusReadHoldingRegisters,    // 0x03
 //modbusReadInputRegister,     // 0x04
 modbusInvalid,               // 0x04
 modbusWriteSingleCoil,         // 0x05
 modbusWriteSingleRegister,     // 0x06
 //modbusReadExceptionStatus,     // 0x07
 modbusInvalid,             // 0x07
 modbusDiagnostic,          // 0x08
 modbusInvalid,             // 0x09
 modbusInvalid,             // 0x0a
 //modbusGetComEventCounter,      // 0x0b
 modbusInvalid,             // 0x0b
 //modbusGetComEventLog,          // 0x0c
 modbusInvalid,             // 0x0c
 modbusInvalid,             // 0x0d
 modbusInvalid,             // 0x0e
 modbusWriteCoils, // 0x0f
 modbusWriteHoldingRegisters,      // 0x10
 //modbusReportServerID,          // 0x11
 modbusInvalid,             // 0x11
 modbusInvalid,             // 0x12
 modbusInvalid,             // 0x13
 //modbusReadFileRecord,          // 0x14
 modbusInvalid,             // 0x14
 //modbusWriteFileRecord,         // 0x15
 modbusInvalid,             // 0x15
 //modbusMaskWriteRegister,       // 0x16
 modbusInvalid,             // 0x16
 //modbusReadWriteMultipleRegisters,  // 0x17
 modbusInvalid,             // 0x17
 //modbusReadFIFOqueue            // 0x18
 modbusInvalid,             // 0x18
 modbusInvalid,             // 0x19
 modbusInvalid,             // 0x1a
 modbusInvalid,             // 0x1b
 modbusInvalid,             // 0x1c
 modbusInvalid,             // 0x1d
 modbusInvalid,             // 0x1e
 modbusInvalid,             // 0x1f
 modbusInvalid,             // 0x20
 modbusInvalid,             // 0x21
 modbusInvalid,             // 0x22
 modbusInvalid,             // 0x23
 modbusInvalid,             // 0x24
 modbusInvalid,             // 0x25
 modbusInvalid,             // 0x26
 modbusInvalid,             // 0x27
 modbusInvalid,             // 0x28
 modbusInvalid,             // 0x29
 modbusInvalid,             // 0x2a
 modbusDeviceID,            // 0x2b
};

static void (*const diagnosticFuncmap[])(void) =
{
 diagnosticReturnQuery,             // 0x00
 diagnosticRestartCommunication,    // 0x01
 diagnosticInvalid,                 // 0x02 content of diagnostiv register is not defined by standard
 diagnosticInvalid,                 // 0x03 change ASCII Delimiter
                                    // ASCII mode not implemented
 diagnosticForceListenOnly,         // 0x04
 diagnosticInvalid,                 // 0x05
 diagnosticInvalid,                 // 0x06
 diagnosticInvalid,                 // 0x07
 diagnosticInvalid,                 // 0x08
 diagnosticInvalid,                 // 0x09
 diagnosticClear,                   // 0x0a
 diagnosticBusMessageCount,         // 0x0b
 diagnosticBusCommErrorCount,       // 0x0c
 diagnosticBusExceptionCount,       // 0x0d
 diagnosticSlaveMessageCount,       // 0x0e
 diagnosticNoResponseCount,         // 0x0f
 diagnosticNAK_Count,               // 0x10
 diagnosticInvalid,                 // 0x11 slave will never NAK busy
 diagnosticSlaveOverrunCount,       // 0x12 slave overrun
 diagnosticInvalid,                 // 0x13
 diagnosticClearSlaveOverrunCount,  // 0x14 clear slave overrun
};

void modbusInit(void) {
    // if button sw1 is pressed set modbus slave address to 0x01
    if (!gpioReadBit(15)) {
        modbus.slaveAddress = 0x01;
        }

    modbus.listenOnly = 0;

    // null diagnostic counters
    modbusCounter.busMessages = 0;
    modbusCounter.busCommunicationErrors = 0xffff;  // somehow during startup this is increased by 1
    modbusCounter.busExceptionErrors = 0;
    modbusCounter.slaveMessages = 0;
    modbusCounter.slaveNoResponse = 0;
    modbusCounter.slaveNAKs = 0;
    modbusCounter.slaveOverruns = 0;

    // call this before RTOS scheduler is started
    xTaskCreate( vModbus , ( signed char * ) "ModbusSlave", configMINIMAL_STACK_SIZE, NULL, (4), &hModbus );
};

static void vModbus ( void *pvParameters) {
    for(;;)   {
        receiveBuffer();
        // taskYIELD();
        checkAndProcessRequest();
    }
};


/* Write single coil
 * Request
 * Function code       1byte   0x05
 * Output address      2bytes  0x0000 to 0xffff
 * Output value        2bytes  0x0000 or 0xff00

 * Normal response
 * Function code       1byte   0x05
 * Output address      2bytes  0x0000 to 0xffff
 * Read back value     2bytes  0x0000 or 0xff00
*/
void modbusWriteSingleCoil(void) {
    extern struct packetTp packet;
    unsigned char *packetTX_Ptr;
    unsigned int address;
    unsigned int value;


    // if packet length < 4 no address and no output values (+1 for modbus address, +1 for modbus function code)
    if (*packet.RX < 4) {
        modbusException(ILLEGALDATAADDRESS);
        return;
        }
    if (*packet.RX < 6) {
        modbusException(ILLEGALDATAVALUE);
        return;
        }

    // read address and value
    address = (((unsigned int)*(packet.RX+3)) << 8 )+ *(packet.RX+4);
    value = (((unsigned int)*(packet.RX+5)) << 8)+ *(packet.RX+6);

    // value != ( 0x0000 or 0xff00) => exception 03
    if ((value != 0) && (value != 0xff00)) {
        modbusException(ILLEGALDATAVALUE);
        return;
    }

    // address invalid =>exception 0x02
    if (!checkGpioWriteAddress(address)) {
        modbusException(0x02);
        // modbusException(ILLEGALDATAADDRESS);
        return;
    }
    // write output
    gpioWriteBit(address,value >> 15);

    // readback value to verify correct output error => exception 04
    if((gpioReadBit(address) ? 1:0 )!= (value >> 15)){
        modbusException(SERVERDEVICEFAILURE);
        return;
    }

    // send ack
    if (!modbus.broadcast) {
        packetTX_Ptr = packet.TX;

        // reply lenght = 6
        *packetTX_Ptr  = 0x06;
        packetTX_Ptr++;
        // modbuss address
        *packetTX_Ptr  = modbus.slaveAddress;
        packetTX_Ptr++;
        // function code 0x05
        *packetTX_Ptr  = 0x05;
        packetTX_Ptr++;
        // address
        *packetTX_Ptr = address >> 8;
        packetTX_Ptr++;
        *packetTX_Ptr = address & 0xff;
        packetTX_Ptr++;
        // value
        *packetTX_Ptr = value >> 8;
        packetTX_Ptr++;
        *packetTX_Ptr = value & 0xff;
        packetTX_Ptr++;

        sendBuffer();
        }
}

void modbusWriteCoils(void) {
  extern struct packetTp packet;
    unsigned char *packetRX_Ptr;
    unsigned char *packetTX_Ptr;
    unsigned int startAddress;
    unsigned int qtyOfCoils;
    unsigned char byteCount;

    unsigned long coilsWrite;

    unsigned char count;

    // funciton code x0f, 2bytes address, 2bytes qty of coils, 1byte byteCount, N bytes values


    // packet length < 4 no address and no nr of coils was given (+1 for modbus address, +1 for modbus function code)
    if (*packet.RX < 4) {
        modbusException(ILLEGALDATAADDRESS);
        return;
        }
    // no byte count and not a single value
    if (*packet.RX < 7) {
        modbusException(ILLEGALDATAVALUE);
        return;
        }
    // read byte count and check length
    byteCount = *(packet.RX+7);
    if (*packet.RX != byteCount + 7) {
        modbusException (ILLEGALDATAVALUE);
        return;
        }

    // read address and coilsToRead
    startAddress = (((unsigned int)*(packet.RX+3)) << 8 )+ *(packet.RX+4);
    qtyOfCoils = (((unsigned int)*(packet.RX+5)) << 8)+ *(packet.RX+6);

    // address invalid =>exception 0x02
    if (!checkGpioWriteAddress(startAddress)) {
        // modbusException(0x02);
        modbusException(ILLEGALDATAADDRESS);
        return;
        }

    // coilsToWrite invalid => exception 0x03
    if (!checkGpioWriteAddress(startAddress+qtyOfCoils-1)) {    // if one coil is written it is written at startAddress
        modbusException(ILLEGALDATAVALUE);
        return;
        }

    coilsWrite = 0;
    packetRX_Ptr = packet.RX + 8;
    for (count = 0; count <= qtyOfCoils>>3; count++) {
        coilsWrite <<= 8;
        coilsWrite += *packetRX_Ptr;
        packetRX_Ptr++;
        }

    // write coils
    gpioWriteBits(startAddress, qtyOfCoils, coilsWrite);

    // no readback of coils, for now (if inputs are in the write address range the readback will be wrong)

    // send ack
    if (!modbus.broadcast) {
        packetTX_Ptr = packet.TX;

        // reply lenght = 6
        *packetTX_Ptr  = 0x06;
        packetTX_Ptr++;
        // modbuss address
        *packetTX_Ptr  = modbus.slaveAddress;
        packetTX_Ptr++;
        // function code 0x0f
        *packetTX_Ptr  = 0x0f;
        packetTX_Ptr++;
        // address
        *packetTX_Ptr = startAddress >> 8;
        packetTX_Ptr++;
        *packetTX_Ptr = startAddress & 0xff;
        packetTX_Ptr++;
        // qty of coils
        *packetTX_Ptr = qtyOfCoils >> 8;
        packetTX_Ptr++;
        *packetTX_Ptr = qtyOfCoils & 0xff;
        packetTX_Ptr++;

        sendBuffer();
        }

}


void modbusReadCoils(void) {
    extern struct packetTp packet;
    unsigned char *packetTX_Ptr;
    unsigned int startAddress;
    unsigned int qtyOfCoils;
    unsigned long coilsRead;

    unsigned char count;

    // if packet length < 4 no address and no nr of coils was given (+1 for modbus address, +1 for modbus function code)
    if (*packet.RX < 4) {
        modbusException(ILLEGALDATAADDRESS);
        return;
        }

    if (*packet.RX < 6) {
        modbusException(ILLEGALDATAVALUE);
        return;
        }
    // read address and coilsToRead
    startAddress = (((unsigned int)*(packet.RX+3)) << 8 )+ *(packet.RX+4);
    qtyOfCoils = (((unsigned int)*(packet.RX+5)) << 8)+ *(packet.RX+6);

     // address invalid =>exception 0x02
    if (!checkGpioReadAddress(startAddress)) {
        // modbusException(0x02);
        modbusException(ILLEGALDATAADDRESS);
        return;
        }
     // coilsToRead invalid => exception 0x03
     if (!checkGpioReadAddress(startAddress+qtyOfCoils-1)) {    // if one coil is read it is read at startAddress
        modbusException(ILLEGALDATAVALUE);
        return;
        }

    coilsRead = gpioReadBits(startAddress, qtyOfCoils);

    // send ack
    // return packet: modbus address; function code; byte count; coil status
    // byte count = qtyOfCoils >> 8

    packetTX_Ptr = packet.TX;

    // reply lenght = (qtyOfCoils >> 8) + 4
    *packetTX_Ptr  = (qtyOfCoils >> 8) + 4;
    packetTX_Ptr++;
    // modbuss address
    *packetTX_Ptr  = modbus.slaveAddress;
    packetTX_Ptr++;
    // function code 0x01
    *packetTX_Ptr  = 0x01;
    packetTX_Ptr++;
    // Quantity of Outputs / 8, if the remainder is different of 0 => N = N+1
    *packetTX_Ptr = (qtyOfCoils % 8) ? (qtyOfCoils >> 8)+1 : qtyOfCoils >> 8;
    packetTX_Ptr++;
    // values
    for (count = 0; count <= qtyOfCoils>>8; count++) {
        *packetTX_Ptr = coilsRead & 0xff;
        coilsRead >>= 8;
        packetTX_Ptr++;
    }
    sendBuffer();
}

void modbusReadDiscreateInputs (void) {
    extern struct packetTp packet;
    unsigned char *packetTX_Ptr;
    unsigned int startAddress;
    unsigned int qtyOfInputs;
    unsigned long inputsRead;

    unsigned char count;

    // if packet length < 4 no address and no nr of inputs was given (+1 for modbus address, +1 for modbus function code)
    if (*packet.RX < 4) {
        modbusException(ILLEGALDATAADDRESS);
        return;
        }
    if (*packet.RX < 6) {
        modbusException(ILLEGALDATAVALUE);
        return;
        }

    // read address and coilsToRead
    startAddress = (((unsigned int)*(packet.RX+3)) << 8 )+ *(packet.RX+4);
    qtyOfInputs = (((unsigned int)*(packet.RX+5)) << 8)+ *(packet.RX+6);

     // address invalid =>exception 0x02
    if (!checkGpioReadAddress(startAddress)) {
        // modbusException(0x02);
        modbusException(ILLEGALDATAADDRESS);
        return;
        }
     // inputsToRead invalid => exception 0x
     if (!checkGpioReadAddress(startAddress+qtyOfInputs-1)) {    // if one coil is read it is read at startAddress
        modbusException(ILLEGALDATAVALUE);
        return;
        }
    inputsRead = gpioReadBits(startAddress, qtyOfInputs);

    // send ack
    // return packet: modbus address; function code; byte count; coil status
    // byte count = qtyOfInputs >> 8

    packetTX_Ptr = packet.TX;

    // reply lenght = (qtyOfInputss >> 8) + 4
    *packetTX_Ptr  = (qtyOfInputs >> 8) + 4;
    packetTX_Ptr++;
    // modbuss address
    *packetTX_Ptr  = modbus.slaveAddress;
    packetTX_Ptr++;
    // function code 0x02
    *packetTX_Ptr  = 0x02;
    packetTX_Ptr++;
    // Quantity of Outputs / 8, if the remainder is different of 0 => N = N+1
    *packetTX_Ptr = (qtyOfInputs % 8) ? (qtyOfInputs >> 8)+1 : qtyOfInputs >> 8;
    packetTX_Ptr++;
    // values
    for (count = 0; count <= qtyOfInputs>>8; count++) {
        *packetTX_Ptr = inputsRead & 0xff;
        inputsRead >>= 8;
        packetTX_Ptr++;
    }
    sendBuffer();
}


void modbusReadHoldingRegisters (void) {
    extern struct packetTp packet;
    extern const int regsNrOfRegs;
    unsigned char *packetTX_Ptr;
    int startAddress;
    int qtyOfRegistersToRead;
    int readValue;

    int mappedAddress;

    unsigned char count;

    // if packet length < 4 no address and no nr of inputs was given (+1 for modbus address, +1 for modbus function code)
    if (*packet.RX < 4) {
        modbusException(ILLEGALDATAADDRESS);
        return;
        }
    if (*packet.RX < 6) {   // no qty given
        modbusException(ILLEGALDATAVALUE);
        return;
        }
    // read address and coilsToRead
    startAddress = (((unsigned int)*(packet.RX+3)) << 8 )+ *(packet.RX+4);
    qtyOfRegistersToRead = (((unsigned int)*(packet.RX+5)) << 8)+ *(packet.RX+6);

    // map address and check if invalid =>exception 0x02
    mappedAddress = regsSearchAddress (startAddress);
    if (mappedAddress < 0) {
           modbusException(ILLEGALDATAADDRESS);
        return;
        }
    // check for invalid quantity
    if ((mappedAddress + qtyOfRegistersToRead) >  regsNrOfRegs) {
        modbusException(ILLEGALDATAADDRESS);
        return;
        }

    // send ack
    // return packet: modbus address; function code; byte count
    // (byte count = 2 * Qty of registers, registers content);
    packetTX_Ptr = packet.TX;

    // reply lenght
    *packetTX_Ptr  = (qtyOfRegistersToRead << 1) + 3;
    packetTX_Ptr++;
    // modbuss address
    *packetTX_Ptr  = modbus.slaveAddress;
    packetTX_Ptr++;
    // function code 0x03
    *packetTX_Ptr  = 0x03;
    packetTX_Ptr++;
    // byte count
    *packetTX_Ptr = qtyOfRegistersToRead << 1;
    packetTX_Ptr++;
    // values
    for (count = mappedAddress; count < (mappedAddress + qtyOfRegistersToRead); count++) {
        readValue = regsReadSingle(count);
        *packetTX_Ptr  = readValue >> 8;
        packetTX_Ptr++;
        *packetTX_Ptr  = readValue & 0xff;
        packetTX_Ptr++;
    }
    sendBuffer();
}

void modbusWriteSingleRegister (void) {
    extern struct packetTp packet;
    extern const int regsNrOfRegs;
    unsigned char *packetTX_Ptr;
    unsigned char *packetRX_Ptr;
    int address;
    int writeValue;
    unsigned char byteCount;

    int mappedAddress;
    unsigned char count;
    unsigned char currentSlaveAddress;

    currentSlaveAddress = modbus.slaveAddress;


    // function code x06, 2bytes address, 2 bytes values


    // packet length < 4 no address and no data
    if (*packet.RX < 4) {
        modbusException(ILLEGALDATAADDRESS);
        return;
        }
    // no data
    if (*packet.RX < 6) {
        modbusException(ILLEGALDATAVALUE);
        return;
        }

    // read address
    address = (((unsigned int)*(packet.RX+3)) << 8 )+ *(packet.RX+4);

    // map address and check if invalid =>exception 0x02
    mappedAddress = regsSearchAddress (address);
    if (mappedAddress < 0) {
           modbusException(ILLEGALDATAADDRESS);
        return;
        }

    // write register
    packetRX_Ptr = packet.RX + 5;

    writeValue = *packetRX_Ptr << 8;
    packetRX_Ptr++;
    writeValue |= *packetRX_Ptr;
    packetRX_Ptr++;
    regsWriteSingle(mappedAddress, writeValue);

    // send ack
    if (!modbus.broadcast) {
        packetTX_Ptr = packet.TX;
        // reply lenght = 6
        *packetTX_Ptr  = 0x06;
        packetTX_Ptr++;
        // modbuss address
        *packetTX_Ptr  = currentSlaveAddress;
        packetTX_Ptr++;
        // function code 0x06
        *packetTX_Ptr  = 0x06;
        packetTX_Ptr++;
        // address
        *packetTX_Ptr = address >> 8;
        packetTX_Ptr++;
        *packetTX_Ptr = address & 0xff;
        sendBuffer();
        }
}


void modbusWriteHoldingRegisters (void) {
    extern struct packetTp packet;
    extern const int regsNrOfRegs;
    unsigned char *packetTX_Ptr;
    unsigned char *packetRX_Ptr;
    int startAddress;
    int qtyOfRegistersToWrite;
    int writeValue;
    unsigned char byteCount;

    int mappedAddress;
    unsigned char count;
    unsigned char currentSlaveAddress;

    currentSlaveAddress = modbus.slaveAddress;


    // function code x10, 2bytes address, 2bytes qty of registers, 1byte byteCount, N bytes values


    // packet length < 4 no address and no nr of registers was given (+1 for modbus address, +1 for modbus function code)
    if (*packet.RX < 4) {
        modbusException(ILLEGALDATAADDRESS);
        return;
        }
    // no byte count and not a single value
    if (*packet.RX < 7) {
        modbusException(ILLEGALDATAVALUE);
        return;
        }
    // read byte count and check length
    byteCount = *(packet.RX+7);
    if (*packet.RX != byteCount + 7) {
        modbusException (ILLEGALDATAVALUE);
        return;
        }

    // read address and coilsToRead
    startAddress = (((unsigned int)*(packet.RX+3)) << 8 )+ *(packet.RX+4);
    qtyOfRegistersToWrite = (((unsigned int)*(packet.RX+5)) << 8)+ *(packet.RX+6);

    // map address and check if invalid =>exception 0x02
    mappedAddress = regsSearchAddress (startAddress);
    if (mappedAddress < 0) {
           modbusException(ILLEGALDATAADDRESS);
        return;
        }
    // check for invalid quantity
    if ((mappedAddress + qtyOfRegistersToWrite) >  regsNrOfRegs) {
        modbusException(ILLEGALDATAADDRESS);
        return;
        }

    // write registers
    packetRX_Ptr = packet.RX + 8;
    for (count = mappedAddress; count < (mappedAddress + qtyOfRegistersToWrite); count++) {
        writeValue = *packetRX_Ptr << 8;
        packetRX_Ptr++;
        writeValue |= *packetRX_Ptr;
        packetRX_Ptr++;
        regsWriteSingle(count, writeValue);
        }

    // send ack
    if (!modbus.broadcast) {
        packetTX_Ptr = packet.TX;
        // reply lenght = 6
        *packetTX_Ptr  = 0x06;
        packetTX_Ptr++;
        // modbuss address
        *packetTX_Ptr  = currentSlaveAddress;
        packetTX_Ptr++;
        // function code 0x10
        *packetTX_Ptr  = 0x10;
        packetTX_Ptr++;
        // address
        *packetTX_Ptr = startAddress >> 8;
        packetTX_Ptr++;
        *packetTX_Ptr = startAddress & 0xff;
        packetTX_Ptr++;
        // qty of coils
        *packetTX_Ptr = qtyOfRegistersToWrite >> 8;
        packetTX_Ptr++;
        *packetTX_Ptr = qtyOfRegistersToWrite & 0xff;
        packetTX_Ptr++;

        sendBuffer();
        }
}

// response for invalid function (this also covers not implemented functions and functions not part of the standard)
// aka NACK
void modbusException (unsigned char exceptionCode) {
    unsigned char *packetFunctPtr;
    unsigned char *packetTX_Ptr;

    extern struct packetTp packet;
    // extern volatile unsigned char packetRX[];
    // extern volatile unsigned char packetTX[];

    modbusCounter.busExceptionErrors++;
    if (!modbus.broadcast) {
        modbusCounter.slaveNAKs++;

        packetFunctPtr = packet.RX + 2;
        packetTX_Ptr = packet.TX;

        // reply lenght = 3
        *packetTX_Ptr  = 0x03;
        packetTX_Ptr++;
        // modbuss address
        *packetTX_Ptr  = modbus.slaveAddress;
        packetTX_Ptr++;
        // error reply: function code | 0x80
        *packetTX_Ptr = *packetFunctPtr | 0x80;
        packetTX_Ptr++;
        // reply with exception code
        *packetTX_Ptr = exceptionCode;
        sendBuffer();
        }
}

void modbusDiagnostic (void) {
    extern struct packetTp packet;

    unsigned char *packetDataPtr;

    // if packet length < 4 no subfunction was given (+1 for modbus address, +1 for modbus function code)
   if (*packet.RX < 4) {
       modbusException(ILLEGALFUNCTION);
       return;
       }

   if (*packet.RX < 6) {
       modbusException(ILLEGALDATAVALUE);
       return;
       }

    // Validate subfunction code
    // Note that the subfunction code < 0x00ff
    // Invalid => ExceptionCode = 2
    packetDataPtr = packet.RX + 4;
    if (*packetDataPtr > 0x14) {
        diagnosticInvalid();
         return;
        }
    else {

        (*diagnosticFuncmap[*packetDataPtr])();
        return;
        }
};

void diagnosticReturnQuery (void){

    extern struct packetTp packet;

    unsigned char *packetTX_Ptr;
    unsigned char *packetRX_Ptr;
    unsigned char count;

    // Check that there is an even number of test data bytes
    if (*packet.RX % 2) {
       modbusException(ILLEGALDATAVALUE);
       return;
       }

    // send ack
    // return packet: modbus address; function code; test data;
    packetTX_Ptr = packet.TX;
    packetRX_Ptr = packet.RX;

    // reply length is equal to length of received package
    *packetTX_Ptr  = *packetRX_Ptr;
    packetTX_Ptr++;
    // modbus address
    *packetTX_Ptr  = modbus.slaveAddress;
    packetTX_Ptr++;
    // function code 0x08
    *packetTX_Ptr  = 0x08;
    packetTX_Ptr++;
    // subfunction code 0x0000
    *packetTX_Ptr  = 0x00;
    packetTX_Ptr++;
    *packetTX_Ptr  = 0x00;
    packetTX_Ptr++;

    // set RX pointer to start of test data
    packetRX_Ptr += 5;

    // test data values
    for (count = 0; count < (*packet.RX - 4); count++) {
        *packetTX_Ptr = *packetRX_Ptr;
        packetRX_Ptr++;
        packetTX_Ptr++;
        }
    sendBuffer();
};

void diagnosticRestartCommunication (void){
    extern struct packetTp packet;

    unsigned char *packetTX_Ptr;
    unsigned char *packetRX_Ptr;
    unsigned char count;

    // Check that there are two bytes of data
    if (*packet.RX < 6) {
       if (!modbus.listenOnly)
            modbusException(ILLEGALDATAVALUE);
       return;
       }
    // Check that data is 0x0000 or 0xff00
    packetRX_Ptr = packet.RX + 5;
    if ((*packetRX_Ptr != 0x00) &&  (*packetRX_Ptr != 0xff)) {
       if (!modbus.listenOnly)
            modbusException(ILLEGALDATAVALUE);
       return;
    }
    packetRX_Ptr++;

    if (*packetRX_Ptr != 00) {
       if (!modbus.listenOnly)
            modbusException(ILLEGALDATAVALUE);
       return;
    }

    // reset USART
    packetReset();

    // null diagnostic counters
    modbusCounter.busMessages = 0;
    modbusCounter.busCommunicationErrors = 0;
    modbusCounter.busExceptionErrors = 0;
    modbusCounter.slaveMessages = 0;
    modbusCounter.slaveNoResponse = 0;
    modbusCounter.slaveNAKs = 0;
    modbusCounter.slaveOverruns = 0;

    modbus.listenOnly = 0;

    // send ACK (echo request data)
    packetTX_Ptr = packet.TX;
    packetRX_Ptr = packet.RX;
    for (count = 0; count <= *packet.RX; count++) {
        *packetTX_Ptr = *packetRX_Ptr;
        packetRX_Ptr++;
        packetTX_Ptr++;
        }

    sendBuffer();

    };

void diagnosticForceListenOnly (void){
    extern struct packetTp packet;

    unsigned char *packetTX_Ptr;
    unsigned char *packetRX_Ptr;
    unsigned char count;

    // Check that there are two bytes of data
    if (*packet.RX < 6) {
       modbusException(ILLEGALDATAVALUE);
       return;
       }
    // Check that data is 0x0000
    packetRX_Ptr = packet.RX + 5;
    if (*packetRX_Ptr != 0x00)  {
       modbusException(ILLEGALDATAVALUE);
       return;
    }
    packetRX_Ptr++;

    if (*packetRX_Ptr != 00) {
       modbusException(ILLEGALDATAVALUE);
       return;
    }
    modbus.listenOnly = 0xff;

    // now we are in listen only mode, do net send a response

    };
void diagnosticClear (void){
   extern struct packetTp packet;

    unsigned char *packetTX_Ptr;
    unsigned char *packetRX_Ptr;
    unsigned char count;

    // Check that there are two bytes of data
    if (*packet.RX < 6) {
       modbusException(ILLEGALDATAVALUE);
       return;
       }
    // Check that data is 0x0000
    packetRX_Ptr = packet.RX + 5;
    if (*packetRX_Ptr != 0x00)  {
       modbusException(ILLEGALDATAVALUE);
       return;
    }
    packetRX_Ptr++;
    if (*packetRX_Ptr != 00) {
       modbusException(ILLEGALDATAVALUE);
       return;
    }

    // null diagnostic counters
    modbusCounter.busMessages = 0;
    modbusCounter.busCommunicationErrors = 0;
    modbusCounter.busExceptionErrors = 0;
    modbusCounter.slaveMessages = 0;
    modbusCounter.slaveNoResponse = 0;
    modbusCounter.slaveNAKs = 0;
    modbusCounter.slaveOverruns = 0;
    // send ACK (echo request data 0x0000)
    packetTX_Ptr = packet.TX;
    packetRX_Ptr = packet.RX;
    for (count = 0; count <= *packet.RX; count++) {
        *packetTX_Ptr = *packetRX_Ptr;
        packetRX_Ptr++;
        packetTX_Ptr++;
        }

    sendBuffer();
    };


void diagnosticBusMessageCount (void){
    extern struct packetTp packet;

    unsigned char *packetTX_Ptr;
    unsigned char *packetRX_Ptr;
    unsigned char count;

    // Check that there are two bytes of data
    if (*packet.RX < 6) {
       modbusException(ILLEGALDATAVALUE);
       return;
       }
    // Check that data is 0x0000
    packetRX_Ptr = packet.RX + 5;
    if (*packetRX_Ptr != 0x00)  {
       modbusException(ILLEGALDATAVALUE);
       return;
    }
    packetRX_Ptr++;
    if (*packetRX_Ptr != 00) {
       modbusException(ILLEGALDATAVALUE);
       return;
    }

    // send ACK (echo request data 0x0000)
    packetTX_Ptr = packet.TX;
    packetRX_Ptr = packet.RX;
    for (count = 0; count <= *packet.RX -2; count++) {
        *packetTX_Ptr = *packetRX_Ptr;
        packetRX_Ptr++;
        packetTX_Ptr++;
        }
     *packetTX_Ptr = modbusCounter.busMessages >> 8;
     packetTX_Ptr++;
     *packetTX_Ptr = modbusCounter.busMessages & 0xff;
     packetTX_Ptr++;

    sendBuffer();
    };


void diagnosticBusCommErrorCount (void){
    extern struct packetTp packet;

    unsigned char *packetTX_Ptr;
    unsigned char *packetRX_Ptr;
    unsigned char count;

    // Check that there are two bytes of data
    if (*packet.RX < 6) {
       modbusException(ILLEGALDATAVALUE);
       return;
       }
    // Check that data is 0x0000
    packetRX_Ptr = packet.RX + 5;
    if (*packetRX_Ptr != 0x00)  {
       modbusException(ILLEGALDATAVALUE);
       return;
    }
    packetRX_Ptr++;
    if (*packetRX_Ptr != 00) {
       modbusException(ILLEGALDATAVALUE);
       return;
    }

    // send ACK (echo request data 0x0000)
    packetTX_Ptr = packet.TX;
    packetRX_Ptr = packet.RX;
    for (count = 0; count <= *packet.RX -2; count++) {
        *packetTX_Ptr = *packetRX_Ptr;
        packetRX_Ptr++;
        packetTX_Ptr++;
        }
     *packetTX_Ptr = modbusCounter.busCommunicationErrors >> 8;
     packetTX_Ptr++;
     *packetTX_Ptr = modbusCounter.busCommunicationErrors & 0xff;
     packetTX_Ptr++;

    sendBuffer();
    };


void diagnosticBusExceptionCount (void){
    extern struct packetTp packet;

    unsigned char *packetTX_Ptr;
    unsigned char *packetRX_Ptr;
    unsigned char count;

    // Check that there are two bytes of data
    if (*packet.RX < 6) {
       modbusException(ILLEGALDATAVALUE);
       return;
       }
    // Check that data is 0x0000
    packetRX_Ptr = packet.RX + 5;
    if (*packetRX_Ptr != 0x00)  {
       modbusException(ILLEGALDATAVALUE);
       return;
    }
    packetRX_Ptr++;
    if (*packetRX_Ptr != 00) {
       modbusException(ILLEGALDATAVALUE);
       return;
    }

    // send ACK (echo request data 0x0000)
    packetTX_Ptr = packet.TX;
    packetRX_Ptr = packet.RX;
    for (count = 0; count <= *packet.RX -2; count++) {
        *packetTX_Ptr = *packetRX_Ptr;
        packetRX_Ptr++;
        packetTX_Ptr++;
        }
     *packetTX_Ptr = modbusCounter.busExceptionErrors >> 8;
     packetTX_Ptr++;
     *packetTX_Ptr = modbusCounter.busExceptionErrors & 0xff;
     packetTX_Ptr++;

    sendBuffer();
   };

void diagnosticSlaveMessageCount (void){
    extern struct packetTp packet;

    unsigned char *packetTX_Ptr;
    unsigned char *packetRX_Ptr;
    unsigned char count;

    // Check that there are two bytes of data
    if (*packet.RX < 6) {
       modbusException(ILLEGALDATAVALUE);
       return;
       }
    // Check that data is 0x0000
    packetRX_Ptr = packet.RX + 5;
    if (*packetRX_Ptr != 0x00)  {
       modbusException(ILLEGALDATAVALUE);
       return;
    }
    packetRX_Ptr++;
    if (*packetRX_Ptr != 00) {
       modbusException(ILLEGALDATAVALUE);
       return;
    }

    // send ACK (echo request data 0x0000)
    packetTX_Ptr = packet.TX;
    packetRX_Ptr = packet.RX;
    for (count = 0; count <= *packet.RX -2; count++) {
        *packetTX_Ptr = *packetRX_Ptr;
        packetRX_Ptr++;
        packetTX_Ptr++;
        }
     *packetTX_Ptr = modbusCounter.slaveMessages >> 8;
     packetTX_Ptr++;
     *packetTX_Ptr = modbusCounter.slaveMessages & 0xff;
     packetTX_Ptr++;

    sendBuffer();
    };


void diagnosticNoResponseCount (void){          // broadcasting not implemented so far
    extern struct packetTp packet;

    unsigned char *packetTX_Ptr;
    unsigned char *packetRX_Ptr;
    unsigned char count;

    // Check that there are two bytes of data
    if (*packet.RX < 6) {
       modbusException(ILLEGALDATAVALUE);
       return;
       }
    // Check that data is 0x0000
    packetRX_Ptr = packet.RX + 5;
    if (*packetRX_Ptr != 0x00)  {
       modbusException(ILLEGALDATAVALUE);
       return;
    }
    packetRX_Ptr++;
    if (*packetRX_Ptr != 00) {
       modbusException(ILLEGALDATAVALUE);
       return;
    }

    // send ACK (echo request data 0x0000)
    packetTX_Ptr = packet.TX;
    packetRX_Ptr = packet.RX;
    for (count = 0; count <= *packet.RX -2; count++) {
        *packetTX_Ptr = *packetRX_Ptr;
        packetRX_Ptr++;
        packetTX_Ptr++;
        }
     *packetTX_Ptr = modbusCounter.slaveNoResponse >> 8;
     packetTX_Ptr++;
     *packetTX_Ptr = modbusCounter.slaveNoResponse & 0xff;
     packetTX_Ptr++;

    sendBuffer();
    };

void diagnosticNAK_Count (void){
    extern struct packetTp packet;

    unsigned char *packetTX_Ptr;
    unsigned char *packetRX_Ptr;
    unsigned char count;

    // Check that there are two bytes of data
    if (*packet.RX < 6) {
       modbusException(ILLEGALDATAVALUE);
       return;
       }
    // Check that data is 0x0000
    packetRX_Ptr = packet.RX + 5;
    if (*packetRX_Ptr != 0x00)  {
       modbusException(ILLEGALDATAVALUE);
       return;
    }
    packetRX_Ptr++;
    if (*packetRX_Ptr != 00) {
       modbusException(ILLEGALDATAVALUE);
       return;
    }

    // send ACK (echo request data 0x0000)
    packetTX_Ptr = packet.TX;
    packetRX_Ptr = packet.RX;
    for (count = 0; count <= *packet.RX -2; count++) {
        *packetTX_Ptr = *packetRX_Ptr;
        packetRX_Ptr++;
        packetTX_Ptr++;
        }
     *packetTX_Ptr = modbusCounter.slaveNAKs >> 8;
     packetTX_Ptr++;
     *packetTX_Ptr = modbusCounter.slaveNAKs & 0xff;
     packetTX_Ptr++;

    sendBuffer();
    };

void diagnosticSlaveOverrunCount (void){
    extern struct packetTp packet;

    unsigned char *packetTX_Ptr;
    unsigned char *packetRX_Ptr;
    unsigned char count;

    // Check that there are two bytes of data
    if (*packet.RX < 6) {
       modbusException(ILLEGALDATAVALUE);
       return;
       }
    // Check that data is 0x0000
    packetRX_Ptr = packet.RX + 5;
    if (*packetRX_Ptr != 0x00)  {
       modbusException(ILLEGALDATAVALUE);
       return;
    }
    packetRX_Ptr++;
    if (*packetRX_Ptr != 00) {
       modbusException(ILLEGALDATAVALUE);
       return;
    }

    // send ACK (echo request data 0x0000)
    packetTX_Ptr = packet.TX;
    packetRX_Ptr = packet.RX;
    for (count = 0; count <= *packet.RX -2; count++) {
        *packetTX_Ptr = *packetRX_Ptr;
        packetRX_Ptr++;
        packetTX_Ptr++;
        }
     *packetTX_Ptr = modbusCounter.slaveOverruns >> 8;
     packetTX_Ptr++;
     *packetTX_Ptr = modbusCounter.slaveOverruns & 0xff;
     packetTX_Ptr++;

    sendBuffer();

    };

void diagnosticClearSlaveOverrunCount (void){
    extern struct packetTp packet;

    unsigned char *packetTX_Ptr;
    unsigned char *packetRX_Ptr;
    unsigned char count;

    // Check that there are two bytes of data
    if (*packet.RX < 6) {
       modbusException(ILLEGALDATAVALUE);
       return;
       }
    // Check that data is 0x0000
    packetRX_Ptr = packet.RX + 5;
    if (*packetRX_Ptr != 0x00)  {
       modbusException(ILLEGALDATAVALUE);
       return;
    }
    packetRX_Ptr++;
    if (*packetRX_Ptr != 00) {
       modbusException(ILLEGALDATAVALUE);
       return;
    }

    modbusCounter.slaveOverruns = 0;

    // send ACK (echo request data 0x0000)
    packetTX_Ptr = packet.TX;
    packetRX_Ptr = packet.RX;
    for (count = 0; count <= *packet.RX; count++) {
        *packetTX_Ptr = *packetRX_Ptr;
        packetRX_Ptr++;
        packetTX_Ptr++;
        }
    sendBuffer();

    };



void modbusDeviceID(void) {
    unsigned char *packetDataPtr;
    unsigned char *packetFunctPtr;
    char* constStringPtr;
    unsigned char *packetTX_Ptr;
    unsigned char packetTXlength;
    extern struct packetTp packet;

    // check for MODBUS Encapsulated Interface subfunction code 0x0e
    packetDataPtr  = packet.RX+3;

    if (*packetDataPtr != 0x0e) { // 0x0e is MEI Type assigned number for Device Identification Interface
        modbusException(ILLEGALDATAVALUE);
        return;
        };

    packetDataPtr++;
    // Device ID code
    // 01: request to get the basic device identification (stream access)
    // 02: request to get the regular device identification (stream access)
    // 03: request to get the extended device identification (stream access)       this is not supported here
    // 04: request to get one specific identification object (individual access)

    packetTXlength = 0;
    packetTX_Ptr = packet.TX + 1;  // reply lenght not known, yet, fill in at end
    switch (*packetDataPtr) {
    case 0x01:
        // send basic device identification
        // reply: function code    (no error)

        /* x2b  function
           x0e  MEI type
           x01  read device ID code
           x82  conformity level
           x00  not (more follows)
           x00  next object IDs
           x03  3 objects to follow
         */
        toTXbuffer("\x07\x2b\x0e\x01\x82\x00\x00\x03");
        appendToTXbuffer("\x01\x00");       // object vendor name
        appendToTXbuffer(VENDORNAME);
        appendToTXbuffer("\x01\x01");       // object product code
        appendToTXbuffer(PRODUCTCODE);
        appendToTXbuffer("\x01\x02");       // object major minor revision
        appendToTXbuffer(MAJORMINORREVISION);

        PORTR.OUT ^= 0x01;      // toggle LED 0
        sendBuffer();
        break;

    case 0x02:
        // send reguar device identification
        // reply: function code    (no error)

        /* x2b  function
           x0e  MEI type
           x02  read device ID code
           x82  conformity level
           x00  not (more follows)
           x00  next object IDs
           x03  3 objects to follow
         */
        toTXbuffer("\x07\x2b\x0e\x02\x82\x00\x00\x03");
        appendToTXbuffer("\x01\x00");       // object vendor name
        appendToTXbuffer(VENDORNAME);
        appendToTXbuffer("\x01\x01");       // object product code
        appendToTXbuffer(PRODUCTCODE);
        appendToTXbuffer("\x01\x02");       // object major minor revision
        appendToTXbuffer(MAJORMINORREVISION);
        appendToTXbuffer("\x01\x03");       // object vendor URL
        appendToTXbuffer(VENDORURL);
        appendToTXbuffer("\x01\x04");       // object product name
        appendToTXbuffer(PRODUCTNAME);
        appendToTXbuffer("\x01\x05");       // object model name
        appendToTXbuffer(MODELNAME);
        appendToTXbuffer("\x01\x06");       // object user application name
        appendToTXbuffer(USERAPPLICATIONNAME);

        PORTR.OUT ^= 0x01;      // toggle LED 0
        sendBuffer();
        break;

    case 0x04:
        // send specific identifcation object (not implemented)
        modbusException(ILLEGALDATAVALUE);
        break;

    default:
        // illegal ID code 0x00, 0x03 or > 0x04
        modbusException(ILLEGALDATAVALUE);
        // modbusException(0xf0);
        return;
    }


}

void modbusInvalid(void) {
    modbusException(ILLEGALFUNCTION);
};

void diagnosticInvalid(void) {
    modbusException(ILLEGALFUNCTION);
};


void checkAndProcessRequest(void) {

    extern struct packetTp packet;

    unsigned int packetLength;
    unsigned char *packetDataPtr;
    unsigned char broadcast;

    // extern volatile unsigned char packetRX[];
    // extern volatile unsigned char packetRXerror;
    if (*packet.RX > 0)
        packetLength = *packet.RX;   // CRC already stripped
    else {
        modbusCounter.busCommunicationErrors++;     // do not send exception for telegrams shorter than 3 bytes
        return;
        }
    packetDataPtr = packet.RX + 1;
    // reminder: RX buffer:   packet length | slave address | function code | data address | data value

    // RX error (parity, frame, overrun, CRC) => increase error counter(s)
    if (packet.RXerror & 0xf7) {
            modbusCounter.busCommunicationErrors++;
            return;
            }

    if (packet.RXerror & 0x08)                   // RX buffer overrun
            modbusCounter.slaveOverruns += packet.RXcount - MAXPACKETSIZE;

    modbusCounter.busMessages++;

    // this slave is not addressed => skip
    if ((*packetDataPtr != modbus.slaveAddress) && (*packetDataPtr !=0))
        return;
    if (*packetDataPtr == 0) {
        modbus.broadcast = 1;
        modbusCounter.slaveNoResponse++;
        }
    else {
       modbus.broadcast = 0;
       modbusCounter.slaveMessages++;
        }

    if (modbus.listenOnly) {
        // only function 0x08 subfunction 0x01 (reset communication)
        // exits listen only mode
        packetDataPtr++;
        if (*packetDataPtr != 0x08)
            return;
        packetDataPtr++;
        if (*packetDataPtr != 0x00)
            return;
        packetDataPtr++;
        if (*packetDataPtr != 0x01)
            return;


        // exit listen only mode and reset com
        modbusCounter.slaveMessages++;
        diagnosticRestartCommunication();
        return;
        }

    // Validate function code
    // Invalid => ExceptionCode = 2
    packetDataPtr++;
    if (*packetDataPtr > 0x2b) {
        modbusInvalid();
        return;
        }

    // faunction code allowed when broadcast?
    if (broadcast) {
        if ((*packetDataPtr <5) | (*packetDataPtr >16)) {
            modbusInvalid();
            return;
            }
        if ((*packetDataPtr >6) | (*packetDataPtr <15)) {
            modbusInvalid();
            return;
            }
        }

    (*modbusFuncmap[*packetDataPtr])();

    }


