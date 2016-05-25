#ifndef BOOT_H
#define BOOT_H

#include "include.h"
#include "init.h"
#include "sched.h"
#include "irq.h"
#include "hardware.h"

void OSInit(void); 	// (OS Initialization)
void OSStart(void);

#endif
