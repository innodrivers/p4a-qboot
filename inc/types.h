#ifndef _TYPES_H
#define _TYPES_H

#ifndef __cplusplus
typedef int bool;
#endif


typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;
typedef unsigned long		uint32_t;
typedef unsigned long long	uint64_t;
typedef unsigned int		size_t;
typedef signed int			ssize_t;

typedef enum {
	false,
	true,
}bool_t;

typedef uint8_t			__u8, u8;
typedef uint16_t		__u16, u16;
typedef uint32_t		__u32, u32;

typedef long long     off_t;
typedef unsigned long long loff_t;
typedef unsigned long long lsize_t;


#ifndef NULL
#define NULL	(void*)0
#endif

typedef long intptr_t;
typedef unsigned long uintptr_t;


typedef int status_t;

typedef uintptr_t addr_t;
typedef uintptr_t vaddr_t;
typedef uintptr_t paddr_t;

typedef int kobj_id;

typedef unsigned long time_t;
typedef unsigned long long bigtime_t;
#define INFINITE_TIME ULONG_MAX


#endif	//_TYPES_H

