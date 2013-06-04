#ifndef _NAND_H
#define _NAND_H

/*------------------------------------------------------------------*/
enum {
	HWECC_OFF = 0,
	HWECC_8BITS,
	HWECC_15BITS,
	HWECC_24BITS
};


/*------------------------------------------------------------------*/

// standard nand commands
#define NAND_CMD_RESET			0xff
#define NAND_CMD_READID			0x90
#define NAND_CMD_READ0			0x00
#define NAND_CMD_READ1			0x01
#define NAND_CMD_READSTART		0x30
#define NAND_CMD_READOOB		0x50
#define NAND_CMD_STATUS			0x70

#define NAND_CMD_SEQIN			0x80
#define NAND_CMD_PAGEPROG		0x10
#define NAND_CMD_ERASE1			0x60
#define NAND_CMD_ERASE2			0xd0
#define NAND_CMD_NONE			(-1)



/* status bits */
#define NAND_STATUS_FAIL	0x01
#define NAND_STATUS_FAIL_N1	0x02
#define NAND_STATUS_TRUE_READY	0x20
#define NAND_STATUS_READY	0x40
#define NAND_STATUS_WP		0x80


struct nandflash_info {
	uint32_t pagesize;
	uint32_t blocksize;
	uint32_t chipsize;	/* unit : MiB */

	uint32_t buswidth16;
	uint32_t chips;

	uint32_t oobsize;
};


enum erase_event {
	ERASE_EVENT_PROGRESS = 0,
	ERASE_EVENT_FAIL,
};

struct erase_info {
	loff_t	from;
	lsize_t	len;

	loff_t	fail_addr;
	unsigned progress;

	unsigned long done;
	unsigned long fail;
	unsigned long total;

	void (*callback)(struct erase_info* self, enum erase_event _event);
};

struct program_info {
	loff_t	from;
	size_t	len;
	size_t	retlen;
	const void*	 buf;

	int		withoob;

	void (*callback)(struct program_info* self, unsigned done, unsigned total);
};

struct read_info {
	loff_t	from;	/* offset to read from */
	size_t	len;	/* number of bytes to read */
	size_t	retlen;	/* number of read bytes */
	void*	 buf;	/* the data buffer to put data */

	int		withoob;

	void (*callback)(struct read_info* self, int done, int total);
};

struct nandflash_info* nand_flash_get_flashinfo(void);
int nand_flash_set_flashinfo(struct nandflash_info* info);

int nand_flash_hwecc(int on);
int nand_flash_readid(uint8_t *IDs, int num);

int nand_flash_erase(struct erase_info* ei);
int nand_flash_read(struct read_info* ri);
int nand_flash_write(struct program_info* pi);

int nand_flash_readoob(uint8_t *buff, unsigned long offset, size_t len);




#endif	/* _NAND_H */


