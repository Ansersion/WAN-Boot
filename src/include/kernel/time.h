#ifndef TIME_H
#define TIME_H

#include "include.h"
#include "irq.h"

INT32U GetTime(void);
void delayMs(volatile INT32U ms); // The argument can't be too large

#endif
