#include <types.h>
#include <common.h>
#include <timer.h>

void udelay(unsigned long usec)
{
	unsigned long kv;
	
	do {
		kv = usec > (10*1000*1000) ? (10*1000*1000) : usec;
		__udelay(kv);
		usec -= kv;
	}while(usec);
}
