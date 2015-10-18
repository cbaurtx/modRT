This software is intended for the Xmega Xplained board from Atmel

It is based on FreeRTOS (www.freertos.org) and uses the follwing Xmega port:
https://github.com/yuriykulikov/FreeRTOS-on-XMEGA
FreeRTOS uses a modified GPL License: http://www.freertos.org/a00114.html#exception

The code of this project is governed by the MIT License (see file License.MIT).

Toolchain:

Windows: I use a wimpy laptop and installing The ATMEL IDE is not an option. winavr has some tools
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

<TABLE BORDER="5">
<TH COLSPAN="4">
         <H3><BR>Connector J1: modbus</H3>
      </TH>
 <TR><TD>Funtion</TD><TD>J1 Label</TD><TD>J1Pin</TD><TD>ATXMEGA</TD></TR>
 <TR><TD>RX</TD><TD>RX</TD><TD>3</TD><TD>PC2</TD></TR>
 <TR><TD>TX</TD><TD>TX</TD><TD>4</TD><TD>PC3</TD></TR>
 <TR><TD>TX driver enable (high = enable), also used for TX LED</TD><TD>SS</TD><TD>5</TD><TD>PC4</TD></TR>
 <TR><TD>RX receiver enable (low=enable)</TD><TD>MOSI</TD><TD>6</TD><TD>PC5</TD></TR>
 <TR><TD>modbus Error LED (red, low  = error)</TD><TD>MISO</TD><TD>7</TD><TD>PC6</TD></TR>
 <TR><TD>modbus RX LED (yellow, low = receive packet)</TD><TD>SCK</TD><TD>8</TD><TD>PC7</TD></TR>
</TABLE>

Pins 1 and 2 (ATXMEGA PC0 and PC1) are not used by modbus and are free for user I2C

Please not that RS485 operation was not tested (I do not have access to RS485 hardware). However software
timing was verified by means of an oscilloscope.

Connector J4: error codes
To send error messages independently of any other hardware (does not use DMA or Int). This was used at
the beginning for debugging purposes.

RX,TX           USARTEO

<TABLE BORDER="5">
<TH COLSPAN="2">
         <H3><BR>LEDs and Buttons</H3>
      </TH>
<TR><TD>Function</TD><TD>ATXMEGA</TD></TR>
<TR><TD>LED0</TD><TD>PR0</TD></TR>
<TR><TD>LED1</TD><TD>PR1</TD></TR>
<TR><TD>Status LED</TD><TD>PD4</TD></TR>
<TR><TD>Power LED</TD><TD>PD5</TD></TR>
<TR><TD>Button0</TD><TD>PE5</TD></TR>
<TR><TD>Button1</TD><TD>PF1</TD></TR>
<TR><TD>Button2</TD><TD>PF2</TD></TR>
</TABLE>

Modbus
Implemeted modbus functions:
0x01, 0x02, 0x03, 0x05, 0x08, 0x0f, 0x10, 0x2b
Note that function 0x08, subfunction 0x11 (return slave busy count) is not implemented.

Supported features:
Broadcast, listen only mode

Not implemeted modbus functions:
0x04, 0x06, 0x07, 0x0b, 0x0c, 0x11, 0x14, 0x15, 0x17, 0x18


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
 |-Xmega - FreeRTOSatxmega256a3bu -|-doc
                                   |-License
                                   |-Source
                                   |-modbus -|-hardware
                                             |-tests
                                             |-include

<TABLE>
<TH COLSPAN="1" align="left">
         <H3><BR>Directory structure</H3>
      </TH>
<TR><TD>-|-avr8-gnu-toolchain-linux_x86 ....</TD></TR>
<TR><TD>&nbsp;|-Xmega - FreeRTOSatxmega256a3bu</TD><TD>-|-doc</TD></TR>
<TR><TD></TD><TD>&nbsp;|-License</TD></TR>
<TR><TD></TD><TD>&nbsp;|-Source</TD></TR>
<TR><TD></TD><TD>&nbsp;|-modbus</TD><TD>-|-hardware</TD></TR>
<TR><TD></TD><TD></TD><TD>&nbsp;|-tests</TD></TR>
<TR><TD></TD><TD></TD><TD>&nbsp;|-include</TD></TR>
</TABLE>


<TABLE>
<TH COLSPAN="1"  align="left">
         <H3><BR>Files</H3>
      </TH>
<TR><TD><BR>Name</TD><TD>Description</TD></TR>
<TR><TD>pr.bat</TD><TD>                                      Windows: batch file to program xmega</TD></TR>
<TR><TD>pr</TD><TD>                                          Linux: shell file to program Xmega</TD></TR>
<TR><TD>testReg.c</TD><TD>                                   Modbus registers to test modbus register operations</TD></TR>
<TR><TD>dma.c</TD><TD>                                       DMA driver</TD></TR>
<TR><TD>eeprom.c</TD><TD>                                    EPROM driver (includes handling NVM)</TD></TR>
<TR><TD>ModbusFreeRTOSatxmega256a3buWindows.geany</TD><TD>   Windows: Geany project file</TD></TR>
<TR><TD>ModbusFreeRTOSatxmega256a3buLinux.geany</TD><TD>     Linux: Geany project file</TD></TR>
<TR><TD>regs.c</TD><TD>                                      Define registers</TD></TR>
<TR><TD>adc.c</TD><TD>                                       ADC driver (example only, not optimzed)</TD></TR>
<TR><TD>modRT.c</TD><TD>                                     Main application</TD></TR>
<TR><TD>readme.txt</TD><TD>                                  This file</TD></TR>
<TR><TD>gpio.c</TD><TD>                                      Digital IO driver</TD></TR>
<TR><TD>LCDst7565r.c</TD><TD>                                LCD driver</TD></TR>
<TR><TD>makefile.linux</TD><TD>                              Linux: makefile</TD></TR>
<TR><TD>makefile.windows</TD><TD>                            Windows: makefile</TD></TR>
<TR><TD>packetSerial.c</TD><TD>                              packet driver, includes UART handling</TD></TR>
<TR><TD>modbus.c</TD></TD><TD></TR>
<TR><TD></TD></TD><TD></TR>
<TR><TD>include/</TD></TD><TD></TR>
<TR><TD>adc.h</TD></TD><TD></TR>
<TR><TD>dma.h</TD></TD><TD></TR>
<TR><TD>eeprom.h</TD></TD><TD></TR>
<TR><TD>font8x16.h</TD><TD>                                  LCD fonts</TD></TR>
<TR><TD>FreeRTOSConfig.h</TD><TD>                            FreeRTOS configuration</TD></TR>
<TR><TD>gpio.h</TD></TD><TD></TR>
<TR><TD>gpioPortMap.h</TD><TD>                               Maps IO ports -- modbus IO</TD></TR>
<TR><TD>LCDst7565r.h</TD></TD><TD></TR>
<TR><TD>modbus.h</TD></TD><TD></TR>
<TR><TD>packetSerial.h</TD></TD><TD></TR>
<TR><TD>regs.h</TD></TD><TD></TR>
<TR><TD>regsMap.h</TD><TD>                                   Maps C strucutres -- modbus registers</TD></TR>
<TR><TD>testReg.h</TD></TD><TD></TR>
<TR><TD><TR><TD>hardware/</TD></TD><TD></TR>
<TR><TD>rtmodrs232.png</TD><TD>                              PNG image of rtsmodrs232.sch</TD></TR>
<TR><TD>rtmodrs232.sch</TD><TD>                              GEDA schematic of RS232 interface used for testing</TD></TR>
<TR><TD>rtmodrs485.png</TD><TD>                              PNG image of rtsmodrs485.sch</TD></TR>
<TR><TD>rtmodrs485.sch</TD><TD>                              GEDA schematic of RS485 interface (not tested)</TD></TR>
<TR><TD>tests/</TD></TD><TD></TR>
<TR><TD>minModbus.py</TD><TD>                                Example python script, needs miminalmodbus and
                                            pyserial modules</TD></TR>
<TR><TD>modbusLED.py</TD><TD>example LED0 flasher</TD></TR>
<TR><TD>testModbusUnderlengthTelegram.py</TD><TD>See below</TD></TR>
<TR><TD><TR><TD>testModbusInvalidFunction.py</TD><TD></TD></TR>
<TR><TD><TR><TD>testModbusInvalidData.py</TD><TD>Not implemented</TD></TR>
<TR><TD><TR><TD>testModbusInvalidCRC.py</TD><TD>Not implemented</TD></TR>
<TR><TD><TR><TD>testModbusListenOnly.py</TD></D>Test listen only mode, not implemented</TD></TR>
<TR><TD><TR><TD>testModbusBroadcast.py</TD><TD>Test broadcasting, not implemented<TD></TR>
<TR><TD><TR><TD>testModbusDiagnostics.py</TD><TD>Test diagnostic counters</TD></TR>
<TR><TD><TR><TD>testModbusSlaveAddress.py</TD><TD>Test reading and changing the slave addrress, not implemented</TD></TR>
<TR><TD><TR><TD>testModbusDeviceID.py</TD><TD>Read device ID, not implemented</TD></TR>
</TABLE>

### Tests with faulty telegrams
The tested modbus slave has address 0x03

1. Telegrams shorter than 4 bytes<br>
   0x01<br>
   0x03,0x00<br>
   0x03,0xff,0x41         (valid CRC-16)

2. Invalid function codes<br>
   0x00, 0x04, 0x07, 0x09-0x0e, 0x11-2a, 0x2c-0xff

3. Invalid data<br>
   Needs to be adapted for every modbus function

4. Invalid checksum<br>

5. Invalid baudrate, frame length, stopbytes, parity<br>
It seams to be almost impossible to change serial settings in the middle of a telegram.
