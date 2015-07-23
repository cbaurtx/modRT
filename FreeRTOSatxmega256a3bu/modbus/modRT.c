/* modRT.c
 *
 * Copyright (C) 2015 Christof Baur
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the license.MIT file for details.
 */


#include "FreeRTOS.h"       // also includes avr/io.h
#include "task.h"

#include "packetSerial.h"
#include "dma.h"
#include "gpio.h"
#include "LCDst7565r.h"
#include "modbus.h"
#include "testReg.h"

/* Task Handles */
xTaskHandle hToggleMyLed1;

/*
 * The idle hook is used to schedule co-routines.
 */
void vApplicationIdleHook( void );

/*
 * LED toggeling tasks:
 */
static void vToggleLED1( void *pvParameters );

void vInitClock32MHzInternal( void );
void vInitClock32MHzCal32kHzExt( void );

/*-----------------------------------------------------------*/

int main( void )
{
    vInitClock32MHzCal32kHzExt();
    eepromInit();
    errorInit();
    gpioInit();
    regsInit();
    dmaInit();
    packetInit();           // make sure that dmaInit() was called, first

    modbusInit();           // this also creates the modbus slave task
    testRegInit();
    LCDst7576init();
    adcInit();

    /* Create the standard demo tasks. */
    xTaskCreate( vToggleLED1 , ( signed char * ) "ToggleMyLed1", configMINIMAL_STACK_SIZE, NULL, (4), &hToggleMyLed1 );

    /* In this port, to use preemptive scheduler define configUSE_PREEMPTION
    as 1 in portmacro.h.  To use the cooperative scheduler define
    configUSE_PREEMPTION as 0. */
    vTaskStartScheduler();

    return 0;
}
/*-----------------------------------------------------------*/
void vApplicationIdleHook( void )
{
    //vCoRoutineSchedule();
}

static void vToggleLED1( void *pvParameters )
{
 for( ;; ) {
     vTaskDelay(55);
     gpioWriteBit(18,!gpioReadBit(18));
     }
}


void vInitClock32MHzCal32kHzExt( void )
{
    // Enable and start external 32768Hz oscillator
    VBAT_CTRL |= VBAT_ACCEN_bm;
    VBAT_CTRL |= VBAT_XOSCEN_bm ;

    OSC.XOSCCTRL = OSC_XOSCSEL1_bm ;            // Select external 32768Hz Oscilator
    OSC.CTRL |= OSC_RC32MEN_bm | OSC_XOSCEN_bm; // Enable the internal 32MHz and external oscillators
    while(!(OSC.STATUS & OSC_RC32MRDY_bm));     // Wait for 32MHz oscillator
    while (! (OSC.STATUS & OSC_XOSCRDY_bm));    // Wait for 32768Hz oscillator

    OSC.DFLLCTRL = OSC_RC32MCREF_XOSC32K_gc ;   // 32 MHz autocalibration
    DFLLRC32M.CTRL = DFLL_ENABLE_bm ;           // Enable DFLL

    // select internal  32Mhz
    asm volatile(
        "movw r30,  %0" "\n\t"
        "ldi  r16,  %2" "\n\t"
        "out  %3, r16" "\n\t"
        "st     Z,  %1"
        :
        : "r" (&CLK.CTRL), "r" (CLK_SCLKSEL_RC32M_gc), "M" (CCP_IOREG_gc), "m" (CCP)
        : "r16", "r30", "r31"
        );
    OSC.CTRL &= ~OSC_RC2MEN_bm;                 // Disable 2Mhz oscillator

}

