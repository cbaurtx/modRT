/* TestReg.h
 *
 * Copyright (C) 2015 Christof Baur
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the license.MIT file for details.
 */

#ifndef TESTREG_H
#define TESTREG_H

struct {
    int a;
    int b;
    int c;
    int dummy;
    int e;
}testReg;

void testRegInit(void);

#endif

