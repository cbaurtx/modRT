/* adc.c
 *
 * Copyright (C) 2015 Christof Baur
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the license.MIT file for details.
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include "adc.h"
#include <avr/interrupt.h>      // if you do not inlcude this there will be no error
                                // but the ADC int will not work (this was painful)!

volatile unsigned int ADCcount;


/* Assumes 32 MHz system clock
   Use ADC A, Channel ADCA1, pin 63
 */


int adcCount;
int adcSum;

void adcInit(void) {
    adcCount = 0;
    adcSum8 = 0;
    adcSum = 0;

    // PR.PRPA &= ~PR_ADC_bm;      // Power up Port A
    // RTOS not running, yet => use delay loop
    // __builtin_avr_delay_cycles (CPU_FREQ / 1000);   // wait 1ms

    // disable ADCA
    ADCA.CTRLA = 0;
    // ADCB.CTRLA  &= ~ADC_ENABLE_bm;

    // calibration
    NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc;
    ADCA.CALL =  pgm_read_byte(PRODSIGNATURES_ADCACAL0);
    NVM_CMD = NVM_CMD_NO_OPERATION_gc;
    NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc;
    ADCA.CALH = pgm_read_byte(PRODSIGNATURES_ADCACAL1);
    NVM_CMD = NVM_CMD_NO_OPERATION_gc;

    // Vref = VCC/ 1.6 with VCC = 3.3V
    ADCA.REFCTRL = ADC_REFSEL_INTVCC_gc | ADC_TEMPREF_bm;
    // prescaler 256
    ADCA.PRESCALER = ADC_PRESCALER_DIV256_gc;

    // Do not use event system, free run CH0 only
    ADCA.EVCTRL=(ADCA.EVCTRL & (~(ADC_EVSEL_gm | ADC_EVACT_gm))) | ADC_EVACT_NONE_gc;

    // max current limit, 12 bit resolution, freerun, high impedance
    ADCA.CTRLB = ADC_CONMODE_bm | ADC_FREERUN_bm | ADC_CURRLIMIT_HIGH_gc | ADC_RESOLUTION_12BIT_gc ;
    // ADCB.CTRLB = ADC_CONMODE_bm | ADC_CURRLIMIT_HIGH_gc | ADC_RESOLUTION_12BIT_gc ;

    // single ended
    ADCA.CH0.CTRL = 1, // ADC_CH_INPUTMODE_SINGLEENDED_gc;
    // input pin 1

    // interrupt upon conversion complete
    ADCA.CH0.INTCTRL = ADC_CH_INTMODE_COMPLETE_gc | ADC_CH_INTLVL_MED_gc;

    ADCA_CH0_MUXCTRL = ADC_CH_MUXPOS_PIN0_gc;       // pin0 LDR, pin1 NTC
    // ADCA.CH0.MUXCTRL = ADC_CH_MUXINT_TEMP_gc;  // muxctrl auf temperature reference

    // Ensure the conversion complete flag is cleared
    ADCA.INTFLAGS = ADC_CH0IF_bm; // 0x01

    // clear interrupt flag
    ADCA.CH0.INTFLAGS  = ADC_CH0IF_bm;  // clear interrupt

    // enable ADCA
    ADCA.CTRLA  |= ADC_ENABLE_bm;

    // RTOS not running, yet => use delay loop
    __builtin_avr_delay_cycles (CPU_FREQ / 1000);   // wait 1ms

    }

ISR (ADCA_CH0_vect) {
    if (adcCount > 6) {
        adcCount = 0;
        adcSum8 = adcSum;
        adcSum= 0;
        // PORTR.OUT =  PORTR_IN ^ 0x02;    // toggle LED1 (for debug)
        }
    adcSum +=  ADCA.CH0RES;
    adcCount++;
    ADCA.CH0.INTFLAGS  = ADC_CH0IF_bm;;  // clear interrupt
    }





