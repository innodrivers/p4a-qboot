#ifndef _UTILS_H_
#define _UTILS_H_
#include <types.h>

typedef unsigned long (*rd_regl_f)(unsigned reg);

/*
 *@brief	polling bits specificed in @bitmask all set within @delay.
 *
 *@param rd_regl		register read function.
 *@param reg			register to be read
 *@param bitmask		polling bit mask
 *@param delay		waiting time in us.
 *
 *@return if polling wait timed out return -1, success return 0.
 */
int polling_bits_set_all(rd_regl_f rd_regl, unsigned reg, int bitmask, time_t delay);

/*
 *@brief	polling bits specificed in @bitmask all clear within @delay.
 *
 *@param rd_regl		register read function.
 *@param reg			register to be read
 *@param bitmask		polling bit mask
 *@param delay		waiting time in us.
 *
 *@return if polling wait timed out return -1, success return 0.
 */
int polling_bits_clear_all(rd_regl_f rd_regl, unsigned reg, int bitmask, time_t delay);

/*
 *@brief	polling bits specificed in @bitmask set at least one within @delay.
 *
 *@param rd_regl		register read function.
 *@param reg			register to be read
 *@param bitmask		polling bit mask
 *@param delay		waiting time in us.
 *
 *@return if polling wait timed out return -1, success return 0.
 */
int polling_bits_set_any(rd_regl_f rd_regl, unsigned reg, int bitmask, time_t delay);

/*
 *@brief	polling bits specificed in @bitmask cleared at least one  within @delay.
 *
 *@param rd_regl		register read function.
 *@param reg			register to be read
 *@param bitmask		polling bit mask
 *@param delay		waiting time in us.
 *
 *@return if polling wait timed out return -1, success return 0.
 */
int polling_bits_clear_any(rd_regl_f rd_regl, unsigned reg, int bitmask, time_t delay);


/*
 *@brief	copy data from IO address to memory buffer.
 *
 *@param dst		destination memory buffer
 *@param io_addr	IO address
 *@param len		data length to be copy
 *@param io_addr_inc	flag indicates if io address increase after read
 *
 *@return none
 */
void memory_copy_from_io(uint8_t* dst, volatile uint32_t*io_addr, size_t len, int io_addr_inc);

/*
 *@brief	copy data from memory buffer to IO address.
 *
 *@param io_addr	IO address
 *@param src		source memory buffer
 *@param len		data length to be copy
 *@param io_addr_inc	flag indicates if io address increase after read
 *
 *@return none
 */
void memory_copy_to_io(volatile uint32_t* io_addr, uint8_t* src, size_t len, int io_addr_inc);

/*
 *@brief	calcuate 16bit XOR checksum.
 *
 *@return 	checksum value.
 */
uint16_t calc_xor16_checksum(uint16_t *buff, size_t len);

int generic_ffs(int x);

#endif	/* _UTILS_H_ */
