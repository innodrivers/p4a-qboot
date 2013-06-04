
#include "utils.h"

#ifndef MIN
#define MIN(a, b)		(((a) < (b)) ? (a) : (b))
#endif

typedef enum {
	SET_ANY = 0,
	SET_ALL,
	CLEAR_ANY,
	CLEAR_ALL
}polling_mode_t;

static int __polling_bits(rd_regl_f rd_regl, unsigned reg, unsigned long bitmask, polling_mode_t mode, time_t delay)
{
	unsigned long val;

//	time_t now = current_time();
//	time_t end = now + delay;
	int	done = 0;

	do {
		val = rd_regl(reg);

		switch (mode) {
		case SET_ANY:
			if (val & bitmask)	done = 1;
			break;
		case SET_ALL:
			if ((val & bitmask) == bitmask)
				done = 1;
			break;
		case CLEAR_ANY:
			if (!(val & bitmask))
				done = 1;
			break;
		case CLEAR_ALL:
			if ((val & bitmask) == 0)
				done = 1;
			break;
		}

		if (done)
			return 0;

//	} while (TIME_LT(current_time(), end));
	} while (1);

	return -1;
}

int polling_bits_set_all(rd_regl_f rd_regl, unsigned reg, int bitmask, time_t delay)
{
	return __polling_bits(rd_regl, reg, bitmask, SET_ALL, delay);
}

int polling_bits_clear_all(rd_regl_f rd_regl, unsigned reg, int bitmask, time_t delay)
{
	return __polling_bits(rd_regl, reg, bitmask, CLEAR_ALL, delay);
}

int polling_bits_set_any(rd_regl_f rd_regl, unsigned reg, int bitmask, time_t delay)
{
	return __polling_bits(rd_regl, reg, bitmask, SET_ANY, delay);
}

int polling_bits_clear_any(rd_regl_f rd_regl, unsigned reg, int bitmask, time_t delay)
{
	return __polling_bits(rd_regl, reg, bitmask, CLEAR_ANY, delay);
}

void memory_copy_from_io(uint8_t* dst, volatile uint32_t*src, size_t len, int io_addr_inc)
{
	int i;

	if (((uint32_t)dst & 0x3) == 0) {
		uint32_t *p = (uint32_t*)dst;
		int m = len >> 2;

		for (i=0; i<m; i++) {
			*(p++) = *src;
			if (io_addr_inc)
				src++;
		}

		len -= (m<<2);
		dst = (uint8_t*)p;
	}

	while (len) {
		uint32_t tmp = *src;
		int size = MIN(4, len);
		len -= size;
		if (io_addr_inc)
			src++;

		while (size) {
			*(dst++) = tmp & 0xff;
			tmp >>= 8;
			size--;
		}

	}

}

void memory_copy_to_io(volatile uint32_t* dst, uint8_t* src, size_t len, int io_addr_inc)
{
	int i;
	uint32_t chunk, scratch;
		

	if(((uint32_t)src & 0x3) == 0) {
		int m = (len >> 2);
		uint32_t *p = (uint32_t*)src;
		for (i=0; i<m; i++) {
			*dst = *(p++);
			if (io_addr_inc)
				dst++;
		}
		len -= (m << 2);
		src = (uint8_t*)p;
	}
	
	chunk = 0;
	scratch = 0;
	while (len) {
		scratch |= *(src++) << (chunk * 8);
		chunk++;
		len--;

		if ((chunk==4) || (len==0)) {
			*dst = scratch;
			if (io_addr_inc)
				dst++;
			chunk = 0;
			scratch = 0;
		}
	}
}

uint16_t calc_xor16_checksum(uint16_t* buff, size_t len)
{
	size_t i;
	size_t wlen = len >> 1;
	uint16_t	result = 0;

	for (i=0; i<wlen; i++) {
		result ^= buff[i];
	}

	return result;
}


int generic_ffs(int x)
{
    int r = 1;

    if (!x)
        return 0;
    if (!(x & 0xffff)) {
        x >>= 16;
        r += 16;
    }
    if (!(x & 0xff)) {
        x >>= 8;
        r += 8;
    }
    if (!(x & 0xf)) {
        x >>= 4;
        r += 4;
    }
    if (!(x & 3)) {
        x >>= 2;
        r += 2;
    }
    if (!(x & 1)) {
        x >>= 1;
        r += 1;
    }
    return r;
}
