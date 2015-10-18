#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# minModbus.py
#
# Copyright (C) 2015 Christof Baur
#
# This software may be modified and distributed under the terms
# of the MIT license.  See the license.MIT file for details.
#
# This is not a test but an example

import minimalmodbus
import serial
import os

minimalmodbus.BAUDRATE = 19200
minimalmodbus.PARITY = 'E'
minimalmodbus.BYTESIZE = 8
minimalmodbus.STOPBITS = 1
minimalmodbus.TIMEOUT = 0.02


def modstring(text):
    for char in text:
        instrument.write_registers(0x102,[ord(char)])

if os.name == "posix":
        port='/dev/ttyUSB0'
else:   # windows
        port='COM5'

instrument = minimalmodbus.Instrument(port, 0x03) # port name, slave address (in decimal)
#instrument.debug = True
count = 0

instrument.write_register(0x80,0x0000)

instrument.write_registers(0x100,[0x0002])
modstring("TEST LINE 1")
instrument.write_registers(0x101,[0x0100])
modstring("test line 2")
instrument.write_registers(0x100,[0x0001])
#
while True:
    instrument.write_bit(17, 0)
    instrument.write_bit(17, 1)
    print("Coil read = ",instrument.read_bit(16,functioncode=2))
    print ("ADC register read = ",instrument.read_registers(0x180,1,functioncode=3)[0]>>4)
    print ("Test register a read = ",hex(instrument.read_registers(0x80,1,functioncode=3)[0]))
    instrument.write_registers(0x83,[count,7])
    instrument.write_register(0x80,0x1234)
    instrument.write_registers(0x101,[0x0100])
    modstring(str(count))

    count += 1

