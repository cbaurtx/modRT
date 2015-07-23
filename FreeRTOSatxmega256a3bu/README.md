This software is intended for the Xmega Xplained board from Atmel

It is based on FreeRTOS (www.freertos.org) and uses the follwing Xmega port:
https://github.com/yuriykulikov/FreeRTOS-on-XMEGA
FreeRTOS uses a modified GPL License: http://www.freertos.org/a00114.html#exception

The code of this project is governed by the MIT License (see file License.MIT).

Toolchain:
Windows:
I use a wimpy laptop and installing The ATMEL IDE is not an option. winavr has some tools
I need, so I installed winavr and the atmel avr 8bit toolchain. Not nice but it works. winavr alone will not work!
As IDE I use Geany.

Linux (Mint):
Install the AVR 8bit tool chain from the ATMEL web site and install the Geany IDE
Install the dfu-programmer. I had to add the following udev rule (new file /etc/udev/rules.d/97-atmel.rules)
SUBSYSTEM=="usb", ATTRS{idVendor}=="03eb", ATTRS{idProduct}=="2fe2", MODE="0660", GROUP="plugdev"

Do not forget to reload the udev rules (udevadm control --reload-rules)


I am not an expert and you will find poor code:
1. Use of external variables
2. Use of #define (and const) on one occasion badly mixed:
   in regsMap.h: #define NR_OF_REGS 8
   const int regsNrOfRegs = NR_OF_REGS;
3. Missing type casts:
   char <=> unsigned char
4. Use of global variables => Code is not re-entrant capable!

Hardware:
Refer to the Atmel hardware guide for connector labels.

Connector J1: modbus

RX, TX          USARTC0                                 RX      (Pin 3) ATXMEGA PC2
                                                        TX      (Pin 4) ATXMEGA PC3
TX driver enable (high = enable)                        SS      (Pin 5) ATXMEGA PC4 also used for TX LED
RX receiver enable (low=enable)                         MOSI    (Pin 6) ATXMEGA PC5
modbus Error LED   (red, low  = error)                  MISO    (Pin 7) ATXMEGA PC6
modbus RX LED      (yellow, low = receive packet)       SCK     (Pin 8) ATXMEGA PC7

Pins 1 and 2 (ATXMEGA PC0 and PC1) are not used by modbus and are free for user I2C

Please not that RS485 operation was not tested (I do not have access to RS485 hardware). However software
timing was verified by means of an oscilloscope.

Connector J4: error codes
To send error messages independently of any other hardware (does not use DMA or Int). This was used at
the beginning for debugging purposes.

RX,TX           USARTEO

LEDS and Buttons
LED0        ATXMEGA PR0
LED1        ATXMEGA PR1
Status LED  ATXMEGA PD4
Power LED   ATXMEGA PD5
Button0     ATXMEGA PE5
Button1     ATXMEGA PF1
Button2     ATXMEGA PF2

Modbus
Implemeted modbus functions:
0x01, 0x02, 0x03, 0x05, 0x0f, 0x10, 0x2b

Not implemeted modbus functions:
0x04, 0x06, 0x07, 0x08, 0x0b, 0x0c, 0x11, 0x14, 0x15, 0x17, 0x18


Register address range (for basic devices) is 0 to 9999
So we arrange registers in blocks of 128 for every module
using registers, e.g. LCDst7565 module addresses 0x0100 to 0x017f.
Address range 0x00 to 0x7f is reserved for modbus internal use
(modbus slave address).

Registers are optionally shadowed by EEPROM. Shadowing only restores
the register content from EEPROM at boot time. Writes to the
register and the EEPROM during modbus register write. You are responsible
for saving changed register data to eeprom when writing directly to register variables.
Keep in mind that EEPROM wears and writes should be kept below 100000 in aggregate.

Modbus slave address
If during start up button sw1 is pressed the modbus address is set to 0x01.
Otherwise the modbus slave address is retrieved from the EEPROM address 0x0000.
The modbus address can be set by a modbuswrite to holding register 0x0000. The
changed address takes effect after the acknowledge.
There is no test if the modbus slave address is set to a valid address (2-247).
Slave address 0x01 should be set by push button sw1 only.

Directory structure:

-|-avr8-gnu-toolchain-linux_x86 ....
 |-Xmega - FreeRTOSatxmega256a3bu -|-License
                                   |-Source
                                   |-modbus -|-hardware
                                             |-tests
                                             |-include

Files

pr.bat                                      Windows: batch file to program xmega
pr                                          Linux: shell file to program Xmega
testReg.c                                   Modbus registers to test modbus register operations
dma.c                                       DMA driver
eeprom.c                                    EPROM driver (includes handling NVM)
ModbusFreeRTOSatxmega256a3buWindows.geany   Windows: Geany project file
ModbusFreeRTOSatxmega256a3buLinux.geany     Linux: Geany project file
regs.c                                      Define registers
adc.c                                       ADC driver (example only, not optimzed)
modRT.c                                     Main application
readme.txt                                  This file
gpio.c                                      Digital IO driver
LCDst7565r.c                                LCD driver
makefile.linux                              Linux: makefile
makefile.windows                            Windows: makefile
packetSerial.c                              packet driver, includes UART handling
modbus.c

include/
adc.h
dma.h
eeprom.h
font8x16.h                                  LCD fonts
FreeRTOSConfig.h                            FreeRTOS configuration
gpio.h
gpioPortMap.h                               Maps IO ports -- modbus IO
LCDst7565r.h
modbus.h
packetSerial.h
regs.h
regsMap.h                                   Maps C strucutres -- modbus registers
testReg.h

hardware/
rtmodrs232.png                              PNG image of rtsmodrs232.sch
rtmodrs232.sch                              GEDA schematic of RS232 interface used for testing
rtmodrs485.png                              PNG image of rtsmodrs485.sch
rtmodrs485.sch                              GEDA schematic of RS485 interface (not tested)

tests/
minModbus.py                                Python script for testing, Needs miminalmodbus and
                                            pyserial modules

