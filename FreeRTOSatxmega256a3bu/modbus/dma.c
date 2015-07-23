/* dma.c
 *
 * Copyright (C) 2015 Christof Baur
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the license.MIT file for details.
 */

#include "dma.h"
#include <avr/io.h>

void dmaInit(void) {
    // reset dma controller
    DMA.CTRL = 0;
    DMA.CTRL = DMA_RESET_bm;
    while ((DMA.CTRL & DMA_RESET_bm) != 0);

    // enable dma controller, disable double buffering,
    // fixed priority ch0 -> ch1 -> ch2 -> ch3 (no round robin)
    DMA.CTRL = DMA_ENABLE_bm | DMA_DBUFMODE_DISABLED_gc | DMA_PRIMODE_CH0123_gc;
    }






