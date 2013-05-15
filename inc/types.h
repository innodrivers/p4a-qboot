#ifndef _TYPES_H
#define _TYPES_H

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


#ifndef NULL
#define NULL	(void*)0
#endif

#endif	//_TYPES_H

