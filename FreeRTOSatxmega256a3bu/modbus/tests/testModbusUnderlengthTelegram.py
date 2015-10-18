#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# testModbusUnderlengthTelegram.py
#
# Copyright (C) 2015 Christof Baur
#
# This software may be modified and distributed under the terms
# of the MIT license.  See the license.MIT file for details.
#
import serial
import time
import os

import binascii

if os.name == "posix":
    ser = serial.Serial(
        port='/dev/ttyUSB0',
        baudrate=19200,
        parity='E',
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS,
        timeout=0.01
        )

else:   # windows
    ser = serial.Serial(
        port='COM5',
        baudrate=19200,
        parity='E',
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS,
        timeout=1
        )

print(ser)

error = 0
   
sendThis =  b'\x03\x08\x00\x0c\x00\x00!\xea'
print ("TX: ",sendThis)
   
ser.write(sendThis)
print("Wait RX")
received = ser.read(0xff)
print ("Communication errors ", received[5])

print ("\nSend 1 byte telegram")    
sendThis =  b'\x01'   
print ("TX: ",sendThis)
   
ser.write(sendThis)
print("Wait RX")
received = ser.read(0xff)
print ("Modbus received:", sendThis, received)

print("Received ", received)

sendThis =  b'\x03\x08\x00\x0c\x00\x00!\xea'
print ("TX: ",sendThis)
   
ser.write(sendThis)
print("Wait RX")
received = ser.read(0xff)
print ("Communication errors ", received[5])


print ("\nSend 2 byte telegram")    
sendThis =  b'\x03\x00'   
print ("TX: ",sendThis)
   
ser.write(sendThis)
print("Wait RX")
received = ser.read(0xff)

print("Received ", received)

sendThis =  b'\x03\x08\x00\x0c\x00\x00!\xea'
print ("TX: ",sendThis)
   
ser.write(sendThis)
print("Wait RX")
received = ser.read(0xff)
print ("Communication errors ", received[5])

print ("\nSend 3 byte telegram (invalid, but no communication error)")    
sendThis =  b'\x03\xff\x41' # valid CRC-16
print ("TX: ",sendThis)
   
ser.write(sendThis)
print("Wait RX")
received = ser.read(0xff)
if (received[1] > 0xf):
    error += 1
    print ("Modbus slave exceptions:", sendThis, received)

print("Received ", received)

sendThis =  b'\x03\x08\x00\x0c\x00\x00!\xea'
print ("TX: ",sendThis)
   
ser.write(sendThis)
print("Wait RX")
received = ser.read(0xff)
print ("Communication errors ", received[5])

print("Done")