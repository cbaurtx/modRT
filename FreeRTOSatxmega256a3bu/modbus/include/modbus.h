/* modbus.h
 *
 * Copyright (C) 2015 Christof Baur
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the license.MIT file for details.
 */

#ifndef MODBUS_H
#define MODBUS_H

#define VENDORNAME  "\x0f\x0e\No Vendor"
#define PRODUCTCODE     "\x10\x0fProduct Code 01"
#define MAJORMINORREVISION  "\x06\x05V0.01"
#define VENDORURL       "\x0e\x0d"
#define PRODUCTNAME     "\x06\x05modRT"
#define MODELNAME       "\x06\x05modRTX"
#define USERAPPLICATIONNAME "\x13\x12Xmega modbus slave"

#define CONFORMITY 0x82

struct {
  unsigned char error;
  unsigned char exception;
  unsigned char listenOnly;
  unsigned char broadcast;
  unsigned char slaveAddress;
} modbus;


// function prototypes
void modbusInit(void);

#endif
