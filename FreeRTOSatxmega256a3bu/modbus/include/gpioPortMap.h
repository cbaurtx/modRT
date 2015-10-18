/* gpioPortMap.h
 *
 * Copyright (C) 2015 Christof Baur
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the license.MIT file for details.
 */

#ifndef GPIOPORTMAP_H
#define GPIOPORTMAP_H

// IO port direction map. Bits set to 1 indicate GPIO output
#define DIR_PORTA  0x08     // bits 0-3 header J3 pins 1-4 (shared with onboard fuinctions), bits 4-7 header J2 pins5-8
#define DIR_PORTB  0x00     // bits 4-7 header J2 pins 1-4, bits 4-7 header J3 pins 5-8
#define DIR_PORTC  0x00     // header J1 pins 1-8  header J1 is completely reserved for RS485 modbus
#define DIR_PORTD  0x0b     // bits 0-3 header J4 pins 5-8 (shared with serial flash), bit4 Red status LED, bit5 Green power LED
#define DIR_PORTE  0x10     // bits 0-3 header J4 pins 1-4, bit4 button0
#define DIR_PORTF  0x08     // bit1 button1, bit2 button2
#define DIR_PORTR  0x03     // bit0 Yellow LED0, bit1 Yellow LED1

// IO control bit7 slew rate limit enable, bit6 invert IO enabble (affects input and output),
// bit5-3 Output control, bit2-0 input control
// Output control: b000 OTEM, b001 BUSKEEPER, b010 PULLDOWN, b011 PULLUP, b100 WIREDOR,
//                 b101 WIREDAND, b110 WIREDORPULL, b111 WIREDANDPULL
// Input control:  b000 BOTHEDGES, b001 RISING, b010 FALLING, b011 LEVEL, b100 Reserved,
//                 b101 Reserved, b110 Reserved, b111 INTPUT_DISABLE           note: sensing for interrups and events
#define IOCTRL_PORTA {0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,}
#define IOCTRL_PORTB {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,}
#define IOCTRL_PORTC {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,}
#define IOCTRL_PORTD {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,}
#define IOCTRL_PORTE {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,}
#define IOCTRL_PORTF {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,}
#define IOCTRL_PORTR {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,}

// address is used as index
// defines function and subaddress of port

#define MAX_OUT_ADDRESS 18
const portWriteMapType mapWriteIO[] =
    {
    {writePortA, 0x10},    // 0     GPIO / ADC4
    {writePortA, 0x20},    // 1     GPIO / ADC5
    {writePortA, 0x40},    // 2     GPIO / ADC6
    {writePortA, 0x80},    // 3     GPIO / ADC7
    {writePortB, 0x01},    // 4     GPIO / ADC0
    {writePortB, 0x02},    // 5     GPIO / ADC1
    {writePortB, 0x04},    // 6     GPIO / ADC2
    {writePortB, 0x08},    // 7     GPIO / ADC3
    {writePortC, 0x01},    // 8     GPIO / I2C SDA
    {writePortC, 0x02},    // 9     GPIO / I2C SCL
    {writePortE, 0x01},    // 10    GPIO / I2C SDA
    {writePortE, 0x02},    // 11    GPIO / I2C SCL
    {writePortE, 0x04},    // 12    GPIO / RX
    {writePortE, 0x08},    // 13    GPIO / TX
    {writePortE, 0x20},    // 14    SW0
    {writePortF, 0x02},    // 15    SW1
    {writePortF, 0x04},    // 16    SW2
    {writePortR, 0x01},    // 17    LED0
    {writePortR, 0x02},    // 18    LED1
    };

#define MAX_IN_ADDRESS 18
const portReadMapType mapReadIO[] =
    {
    {readPortA, 0x10},    // 0     GPIO / ADC4
    {readPortA, 0x20},    // 1     GPIO / ADC5
    {readPortA, 0x40},    // 2     GPIO / ADC6
    {readPortA, 0x80},    // 3     GPIO / ADC7
    {readPortB, 0x01},    // 4     GPIO / ADC0
    {readPortB, 0x02},    // 5     GPIO / ADC1
    {readPortB, 0x04},    // 6     GPIO / ADC2
    {readPortB, 0x08},    // 7     GPIO / ADC3
    {readPortC, 0x01},    // 8     GPIO / I2C SDA
    {readPortC, 0x02},    // 9     GPIO / I2C SCL
    {readPortE, 0x01},    // 10    GPIO / I2C SDA
    {readPortE, 0x02},    // 11    GPIO / I2C SCL
    {readPortE, 0x04},    // 12    GPIO / RX
    {readPortE, 0x08},    // 13    GPIO / TX
    {readPortE, 0x20},    // 14    SW0
    {readPortF, 0x02},    // 15    SW1
    {readPortF, 0x04},    // 16    SW2
    {readPortR, 0x01},    // 17    LED0 (allows to read if LED is on)
    {readPortR, 0x02},    // 18    LED1
    };

#endif

