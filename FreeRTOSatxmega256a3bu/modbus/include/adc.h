/* adc.h
 *
 * Copyright (C) 2015 Christof Baur
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the license.MIT file for details.
 */

#ifndef ADC_H
#define ADC_H

#define CPU_FREQ 32000000   // CPU frequency this must match the actual frequency. Only 32MHz for now

// public function prototypes
void adcInit (void);

int adcSum8;

#endif


