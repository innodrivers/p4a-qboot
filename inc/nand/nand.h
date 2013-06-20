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

/*
 * NAND Flash Manufacturer ID Codes
 */
#define NAND_MFR_TOSHIBA	0x98
#define NAND_MFR_SAMSUNG	0xec
#define NAND_MFR_FUJITSU	0x04
#define NAND_MFR_NATIONAL	0x8f
#define NAND_MFR_RENESAS	0x07
#define NAND_MFR_STMICRO	0x20
#define NAND_MFR_HYNIX		0xad
#define NAND_MFR_MICRON		0x2c
#define NAND_MFR_AMD		0x01

/*
 * Option constants for bizarre disfunctionality and real
 * features.
 */
/* Chip can not auto increment pages */
#define NAND_NO_AUTOINCR	0x00000001
/* Buswitdh is 16 bit */
#define NAND_BUSWIDTH_16	0x00000002
/* Device supports partial programming without padding */
#define NAND_NO_PADDING		0x00000004
/* Chip has cache program function */
#define NAND_CACHEPRG		0x00000008
/* Chip has copy back function */
#define NAND_COPYBACK		0x00000010
/*
 * AND Chip which has 4 banks and a confusing page / block
 * assignment. See Renesas datasheet for further information.
 */
#define NAND_IS_AND		0x00000020
/*
 * Chip has a array of 4 pages which can be read without
 * additional ready /busy waits.
 */
#define NAND_4PAGE_ARRAY	0x00000040
/*
 * Chip requires that BBT is periodically rewritten to prevent
 * bits from adjacent blocks from 'leaking' in altering data.
 * This happens with the Renesas AG-AND chips, possibly others.
 */
#define BBT_AUTO_REFRESH	0x00000080
/*
 * Chip does not require ready check on read. True
 * for all large page devices, as they do not support
 * autoincrement.
 */
#define NAND_NO_READRDY		0x00000100
/* Chip does not allow subpage writes */
#define NAND_NO_SUBPAGE_WRITE	0x00000200

/* Device is one of 'new' xD cards that expose fake nand command set */
#define NAND_BROKEN_XD		0x00000400

/* Device behaves just like nand, but is readonly */
#define NAND_ROM		0x00000800

/* Options valid for Samsung large page devices */
#define NAND_SAMSUNG_LP_OPTIONS \
	(NAND_NO_PADDING | NAND_CACHEPRG | NAND_COPYBACK)

/* Mask to zero out the chip options, which come from the id table */
#define NAND_CHIPOPTIONS_MSK	(0x0000ffff & ~NAND_NO_AUTOINCR)


/* Cell info constants */
#define NAND_CI_CHIPNR_MSK	0x03
#define NAND_CI_CELLTYPE_MSK	0x0C

#define NAND_MAX_CHIPS 8
/**
 * struct nand_bbt_descr - bad block table descriptor
 * @options:	options for this descriptor
 * @pages:	the page(s) where we find the bbt, used with option BBT_ABSPAGE
 *		when bbt is searched, then we store the found bbts pages here.
 *		Its an array and supports up to 8 chips now
 * @offs:	offset of the pattern in the oob area of the page
 * @veroffs:	offset of the bbt version counter in the oob are of the page
 * @version:	version read from the bbt page during scan
 * @len:	length of the pattern, if 0 no pattern check is performed
 * @maxblocks:	maximum number of blocks to search for a bbt. This number of
 *		blocks is reserved at the end of the device where the tables are
 *		written.
 * @reserved_block_code: if non-0, this pattern denotes a reserved (rather than
 *              bad) block in the stored bbt
 * @pattern:	pattern to identify bad block table or factory marked good /
 *		bad blocks, can be NULL, if len = 0
 *
 * Descriptor for the bad block table marker and the descriptor for the
 * pattern which identifies good and bad blocks. The assumption is made
 * that the pattern and the version count are always located in the oob area
 * of the first block.
 */
struct nand_bbt_descr {
	int options;
	int pages[NAND_MAX_CHIPS];
	int offs;
	int veroffs;
	uint8_t version[NAND_MAX_CHIPS];
	int len;
	int maxblocks;
	int reserved_block_code;
	uint8_t *pattern;
};

/* Options for the bad block table descriptors */

/* The number of bits used per block in the bbt on the device */
#define NAND_BBT_NRBITS_MSK	0x0000000F
#define NAND_BBT_1BIT		0x00000001
#define NAND_BBT_2BIT		0x00000002
#define NAND_BBT_4BIT		0x00000004
#define NAND_BBT_8BIT		0x00000008
/* The bad block table is in the last good block of the device */
#define NAND_BBT_LASTBLOCK	0x00000010
/* The bbt is at the given page, else we must scan for the bbt */
#define NAND_BBT_ABSPAGE	0x00000020
/* The bbt is at the given page, else we must scan for the bbt */
#define NAND_BBT_SEARCH		0x00000040
/* bbt is stored per chip on multichip devices */
#define NAND_BBT_PERCHIP	0x00000080
/* bbt has a version counter at offset veroffs */
#define NAND_BBT_VERSION	0x00000100
/* Create a bbt if none exists */
#define NAND_BBT_CREATE		0x00000200
/* Search good / bad pattern through all pages of a block */
#define NAND_BBT_SCANALLPAGES	0x00000400
/* Scan block empty during good / bad block scan */
#define NAND_BBT_SCANEMPTY	0x00000800
/* Write bbt if neccecary */
#define NAND_BBT_WRITE		0x00001000
/* Read and write back block contents when writing bbt */
#define NAND_BBT_SAVECONTENT	0x00002000
/* Search good / bad pattern on the first and the second page */
#define NAND_BBT_SCAN2NDPAGE	0x00004000
/* Search good / bad pattern on the last page of the eraseblock */
#define NAND_BBT_SCANLASTPAGE	0x00008000
/* Chip stores bad block marker on BOTH 1st and 6th bytes of OOB */
#define NAND_BBT_SCANBYTE1AND6 0x00100000
/* The nand_bbt_descr was created dynamicaly and must be freed */
#define NAND_BBT_DYNAMICSTRUCT 0x00200000
/* The bad block table does not OOB for marker */
#define NAND_BBT_NO_OOB		0x00400000

/* The maximum number of blocks to scan for a bbt */
#define NAND_BBT_SCAN_MAXBLOCKS	4

/*
 * Constants for oob configuration
 */
#define NAND_SMALL_BADBLOCK_POS		5
#define NAND_LARGE_BADBLOCK_POS		0
#define ONENAND_BADBLOCK_POS		0



/**
 * struct nand_flash_dev - NAND Flash Device ID Structure
 * @name:	Identify the device type
 * @id:		device ID code
 * @pagesize:	Pagesize in bytes. Either 256 or 512 or 0
 *		If the pagesize is 0, then the real pagesize
 *		and the eraseize are determined from the
 *		extended id bytes in the chip
 * @erasesize:	Size of an erase block in the flash device.
 * @chipsize:	Total chipsize in Mega Bytes
 * @options:	Bitfield to store chip relevant options
 */
struct nand_flash_dev {
	char *name;
	int id;
	unsigned long pagesize;
	unsigned long chipsize;
	unsigned long erasesize;
	unsigned long options;
};

/**
 * struct nand_manufacturers - NAND Flash Manufacturer ID Structure
 * @name:	Manufacturer name
 * @id:		manufacturer ID code of device.
*/
struct nand_manufacturers {
	int id;
	char *name;
};

extern struct nand_flash_dev nand_flash_ids[];
extern struct nand_manufacturers nand_manuf_ids[];


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


