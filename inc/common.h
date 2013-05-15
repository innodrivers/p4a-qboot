#ifndef _COMMON_H
#define _COMMON_H

#ifndef MIN
#define MIN(a, b)	(((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#endif

#ifndef MiB
#define MiB(x)		((x)<<20)
#endif

#ifndef KiB
#define KiB(x)		((x)<<10)
#endif

#ifndef MHz
#define MHz(x)		((x)*1000*1000)
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define __INLINE__	inline

extern void udelay(unsigned long us);


#define SUPPORT_SERIAL_TRACE	1

#if (SUPPORT_SERIAL_TRACE)
#define PRINT	printf

#else
#define PRINT	//
#endif


static __INLINE__ void dump_data(const char* buff, int len)
{
#ifdef DUMP_DATA	
	int i;
	for(i=0; i<len; i++) {
		PRINT("%02x ", buff[i]);
		if ((i+1)%16==0)
			PRINT("\n");
	}
	PRINT("\n");
#endif
}

#endif	/* _COMMON_H */
