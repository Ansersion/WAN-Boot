#include "time.h"


INT32U GetTime(void)
{
	return GetTime_API();
}

void delayMs(volatile INT32U ms)
{
	INT32U tmp;
	tmp = GetTime() + ms;
	while(1){
		if(tmp < GetTime()) break;
	}
}
