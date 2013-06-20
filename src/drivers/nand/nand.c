/**
 * @file
 * @brief	p4a qboot nand driver
 * @date	2013/05/29
 * 
 * Copyright (c) 2013 Innofidei Inc. All rights reserved.
 */
#include <types.h>
#include <common.h>
#include <io.h>
#include <hardware.h>
#include <clk.h>
#include <string.h>
#include <p4a-regs.h>
#include <nand/nand.h>
#include <nand/p4a_nand.h>
//#include "utils.h"
#include <bitops.h>

struct nand_priv {
	struct nandflash_info *info;

	int row_cycles;
	int column_cycles;
	int pages_per_block;

	int page_shift;
	int block_shift;
	int chip_shift;
	int pagemask;

	int hwecc;
	// nand controller related
	size_t sector_size;
	size_t hwecc_bytes;
	size_t freeoob_bytes;

	int wOffset;
	int rOffset;

	unsigned int options;

	int phys_erase_shift;
	int bbt_erase_shift;
	uint8_t cellinfo;
	int badblockpos;
	int badblockbits;
	
};

static struct nand_priv *chip;

#define NAND_MAX_PAGESIZE 8192
#define NAND_MAX_OOBSIZE 576
#define MAX_PAGECACHE_SIZE		(NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE)

#define NAND_BUFFER_PHY_ADDRESS 0x47000000

/*
 * P4A NFC Command Queue command
 */
#define		CMDQ_SET_CMD		(0x1)
#define		CMDQ_SET_ADDR		(0x2)
#define		CMDQ_SET_IADDR		(0x3)	//auto increasing address
#define		CMDQ_SINGLE_WR		(0x4)
#define		CMDQ_SINGLE_RD		(0x5)
#define		CMDQ_RUN_DMA		(0x6)
#define		CMDQ_WAIT_RDY		(0x7)
#define		CMDQ_CHECK_STATUS	(0x8)
#define		CMDQ_END_QUEUE		(0xF)

//CMDQ_RUN_DMA command parameter
#define		CMD_DMA_512BYTES		(0<<0)
#define		CMD_DMA_16BYTES			(1<<0)
#define		CMD_DMA_2048BYTES		(2<<0)
#define		CMD_DMA_64BYTES			(3<<0)

#define		CMD_DMA_WRITE				(0<<2)
#define		CMD_DMA_READ				(1<<2)

#define		CMD_DMA_SPAREBUFF_DISABLE	(0<<3)
#define		CMD_DMA_SPAREBUFF_ENABLE	(1<<3)

#define		CMD_DMA_ECC_ON			(0<<4)
#define		CMD_DMA_ECC_OFF			(1<<4)

typedef enum {
	NRET_NOERR = 0,
	NRET_ERROR,
	NRET_TIMEOUT,
}nfc_status_t;

/*
*	Chip ID list
*
*	Name. ID code, pagesize, chipsize in MegaByte, eraseblock size,
*	options
*
*	Pagesize; 0, 256, 512
*	0	get this information from the extended chip ID
+	256	256 Byte page size
*	512	512 Byte page size
*/
struct nand_flash_dev nand_flash_ids[] = {

#ifdef CONFIG_MTD_NAND_MUSEUM_IDS
	{"NAND 1MiB 5V 8-bit",		0x6e, 256, 1, 0x1000, 0},
	{"NAND 2MiB 5V 8-bit",		0x64, 256, 2, 0x1000, 0},
	{"NAND 4MiB 5V 8-bit",		0x6b, 512, 4, 0x2000, 0},
	{"NAND 1MiB 3,3V 8-bit",	0xe8, 256, 1, 0x1000, 0},
	{"NAND 1MiB 3,3V 8-bit",	0xec, 256, 1, 0x1000, 0},
	{"NAND 2MiB 3,3V 8-bit",	0xea, 256, 2, 0x1000, 0},
	{"NAND 4MiB 3,3V 8-bit",	0xd5, 512, 4, 0x2000, 0},
	{"NAND 4MiB 3,3V 8-bit",	0xe3, 512, 4, 0x2000, 0},
	{"NAND 4MiB 3,3V 8-bit",	0xe5, 512, 4, 0x2000, 0},
	{"NAND 8MiB 3,3V 8-bit",	0xd6, 512, 8, 0x2000, 0},

	{"NAND 8MiB 1,8V 8-bit",	0x39, 512, 8, 0x2000, 0},
	{"NAND 8MiB 3,3V 8-bit",	0xe6, 512, 8, 0x2000, 0},
	{"NAND 8MiB 1,8V 16-bit",	0x49, 512, 8, 0x2000, NAND_BUSWIDTH_16},
	{"NAND 8MiB 3,3V 16-bit",	0x59, 512, 8, 0x2000, NAND_BUSWIDTH_16},
#endif

	{"NAND 16MiB 1,8V 8-bit",	0x33, 512, 16, 0x4000, 0},
	{"NAND 16MiB 3,3V 8-bit",	0x73, 512, 16, 0x4000, 0},
	{"NAND 16MiB 1,8V 16-bit",	0x43, 512, 16, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 16MiB 3,3V 16-bit",	0x53, 512, 16, 0x4000, NAND_BUSWIDTH_16},

	{"NAND 32MiB 1,8V 8-bit",	0x35, 512, 32, 0x4000, 0},
	{"NAND 32MiB 3,3V 8-bit",	0x75, 512, 32, 0x4000, 0},
	{"NAND 32MiB 1,8V 16-bit",	0x45, 512, 32, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 32MiB 3,3V 16-bit",	0x55, 512, 32, 0x4000, NAND_BUSWIDTH_16},

	{"NAND 64MiB 1,8V 8-bit",	0x36, 512, 64, 0x4000, 0},
	{"NAND 64MiB 3,3V 8-bit",	0x76, 512, 64, 0x4000, 0},
	{"NAND 64MiB 1,8V 16-bit",	0x46, 512, 64, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 64MiB 3,3V 16-bit",	0x56, 512, 64, 0x4000, NAND_BUSWIDTH_16},

	{"NAND 128MiB 1,8V 8-bit",	0x78, 512, 128, 0x4000, 0},
	{"NAND 128MiB 1,8V 8-bit",	0x39, 512, 128, 0x4000, 0},
	{"NAND 128MiB 3,3V 8-bit",	0x79, 512, 128, 0x4000, 0},
	{"NAND 128MiB 1,8V 16-bit",	0x72, 512, 128, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 128MiB 1,8V 16-bit",	0x49, 512, 128, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 128MiB 3,3V 16-bit",	0x74, 512, 128, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 128MiB 3,3V 16-bit",	0x59, 512, 128, 0x4000, NAND_BUSWIDTH_16},

	{"NAND 256MiB 3,3V 8-bit",	0x71, 512, 256, 0x4000, 0},

	/*
	 * These are the new chips with large page size. The pagesize and the
	 * erasesize is determined from the extended id bytes
	 */
#define LP_OPTIONS (NAND_SAMSUNG_LP_OPTIONS | NAND_NO_READRDY | NAND_NO_AUTOINCR)
#define LP_OPTIONS16 (LP_OPTIONS | NAND_BUSWIDTH_16)

	/*512 Megabit */
	{"NAND 64MiB 1,8V 8-bit",	0xA2, 0,  64, 0, LP_OPTIONS},
	{"NAND 64MiB 1,8V 8-bit",	0xA0, 0,  64, 0, LP_OPTIONS},
	{"NAND 64MiB 3,3V 8-bit",	0xF2, 0,  64, 0, LP_OPTIONS},
	{"NAND 64MiB 3,3V 8-bit",	0xD0, 0,  64, 0, LP_OPTIONS},
	{"NAND 64MiB 1,8V 16-bit",	0xB2, 0,  64, 0, LP_OPTIONS16},
	{"NAND 64MiB 1,8V 16-bit",	0xB0, 0,  64, 0, LP_OPTIONS16},
	{"NAND 64MiB 3,3V 16-bit",	0xC2, 0,  64, 0, LP_OPTIONS16},
	{"NAND 64MiB 3,3V 16-bit",	0xC0, 0,  64, 0, LP_OPTIONS16},

	/* 1 Gigabit */
	{"NAND 128MiB 1,8V 8-bit",	0xA1, 0, 128, 0, LP_OPTIONS},
	{"NAND 128MiB 3,3V 8-bit",	0xF1, 0, 128, 0, LP_OPTIONS},
	{"NAND 128MiB 3,3V 8-bit",	0xD1, 0, 128, 0, LP_OPTIONS},
	{"NAND 128MiB 1,8V 16-bit",	0xB1, 0, 128, 0, LP_OPTIONS16},
	{"NAND 128MiB 3,3V 16-bit",	0xC1, 0, 128, 0, LP_OPTIONS16},
	{"NAND 128MiB 1,8V 16-bit",     0xAD, 0, 128, 0, LP_OPTIONS16},

	/* 2 Gigabit */
	{"NAND 256MiB 1,8V 8-bit",	0xAA, 0, 256, 0, LP_OPTIONS},
	{"NAND 256MiB 3,3V 8-bit",	0xDA, 0, 256, 0, LP_OPTIONS},
	{"NAND 256MiB 1,8V 16-bit",	0xBA, 0, 256, 0, LP_OPTIONS16},
	{"NAND 256MiB 3,3V 16-bit",	0xCA, 0, 256, 0, LP_OPTIONS16},

	/* 4 Gigabit */
	{"NAND 512MiB 1,8V 8-bit",	0xAC, 0, 512, 0, LP_OPTIONS},
	{"NAND 512MiB 3,3V 8-bit",	0xDC, 0, 512, 0, LP_OPTIONS},
	{"NAND 512MiB 1,8V 16-bit",	0xBC, 0, 512, 0, LP_OPTIONS16},
	{"NAND 512MiB 3,3V 16-bit",	0xCC, 0, 512, 0, LP_OPTIONS16},

	/* 8 Gigabit */
	{"NAND 1GiB 1,8V 8-bit",	0xA3, 0, 1024, 0, LP_OPTIONS},
	{"NAND 1GiB 3,3V 8-bit",	0xD3, 0, 1024, 0, LP_OPTIONS},
	{"NAND 1GiB 1,8V 16-bit",	0xB3, 0, 1024, 0, LP_OPTIONS16},
	{"NAND 1GiB 3,3V 16-bit",	0xC3, 0, 1024, 0, LP_OPTIONS16},

	/* 16 Gigabit */
	{"NAND 2GiB 1,8V 8-bit",	0xA5, 0, 2048, 0, LP_OPTIONS},
	{"NAND 2GiB 3,3V 8-bit",	0xD5, 0, 2048, 0, LP_OPTIONS},
	{"NAND 2GiB 1,8V 16-bit",	0xB5, 0, 2048, 0, LP_OPTIONS16},
	{"NAND 2GiB 3,3V 16-bit",	0xC5, 0, 2048, 0, LP_OPTIONS16},

	/* 32 Gigabit */
	{"NAND 4GiB 1,8V 8-bit",	0xA7, 0, 4096, 0, LP_OPTIONS},
	{"NAND 4GiB 3,3V 8-bit",	0xD7, 0, 4096, 0, LP_OPTIONS},
	{"NAND 4GiB 1,8V 16-bit",	0xB7, 0, 4096, 0, LP_OPTIONS16},
	{"NAND 4GiB 3,3V 16-bit",	0xC7, 0, 4096, 0, LP_OPTIONS16},

	/* 64 Gigabit */
	{"NAND 8GiB 1,8V 8-bit",	0xAE, 0, 8192, 0, LP_OPTIONS},
	{"NAND 8GiB 3,3V 8-bit",	0xDE, 0, 8192, 0, LP_OPTIONS},
	{"NAND 8GiB 1,8V 16-bit",	0xBE, 0, 8192, 0, LP_OPTIONS16},
	{"NAND 8GiB 3,3V 16-bit",	0xCE, 0, 8192, 0, LP_OPTIONS16},

	/* 128 Gigabit */
	{"NAND 16GiB 1,8V 8-bit",	0x1A, 0, 16384, 0, LP_OPTIONS},
	{"NAND 16GiB 3,3V 8-bit",	0x3A, 0, 16384, 0, LP_OPTIONS},
	{"NAND 16GiB 1,8V 16-bit",	0x2A, 0, 16384, 0, LP_OPTIONS16},
	{"NAND 16GiB 3,3V 16-bit",	0x4A, 0, 16384, 0, LP_OPTIONS16},

	/* 256 Gigabit */
	{"NAND 32GiB 1,8V 8-bit",	0x1C, 0, 32768, 0, LP_OPTIONS},
	{"NAND 32GiB 3,3V 8-bit",	0x3C, 0, 32768, 0, LP_OPTIONS},
	{"NAND 32GiB 1,8V 16-bit",	0x2C, 0, 32768, 0, LP_OPTIONS16},
	{"NAND 32GiB 3,3V 16-bit",	0x4C, 0, 32768, 0, LP_OPTIONS16},

	/* 512 Gigabit */
	{"NAND 64GiB 1,8V 8-bit",	0x1E, 0, 65536, 0, LP_OPTIONS},
	{"NAND 64GiB 3,3V 8-bit",	0x3E, 0, 65536, 0, LP_OPTIONS},
	{"NAND 64GiB 1,8V 16-bit",	0x2E, 0, 65536, 0, LP_OPTIONS16},
	{"NAND 64GiB 3,3V 16-bit",	0x4E, 0, 65536, 0, LP_OPTIONS16},

	/*
	 * Renesas AND 1 Gigabit. Those chips do not support extended id and
	 * have a strange page/block layout !  The chosen minimum erasesize is
	 * 4 * 2 * 2048 = 16384 Byte, as those chips have an array of 4 page
	 * planes 1 block = 2 pages, but due to plane arrangement the blocks
	 * 0-3 consists of page 0 + 4,1 + 5, 2 + 6, 3 + 7 Anyway JFFS2 would
	 * increase the eraseblock size so we chose a combined one which can be
	 * erased in one go There are more speed improvements for reads and
	 * writes possible, but not implemented now
	 */
	{"AND 128MiB 3,3V 8-bit",	0x01, 2048, 128, 0x4000,
	 NAND_IS_AND | NAND_NO_AUTOINCR |NAND_NO_READRDY | NAND_4PAGE_ARRAY |
	 BBT_AUTO_REFRESH
	},

	{NULL,}
};

/*
*	Manufacturer ID list
*/
struct nand_manufacturers nand_manuf_ids[] = {
	{NAND_MFR_TOSHIBA, "Toshiba"},
	{NAND_MFR_SAMSUNG, "Samsung"},
	{NAND_MFR_FUJITSU, "Fujitsu"},
	{NAND_MFR_NATIONAL, "National"},
	{NAND_MFR_RENESAS, "Renesas"},
	{NAND_MFR_STMICRO, "ST Micro"},
	{NAND_MFR_HYNIX, "Hynix"},
	{NAND_MFR_MICRON, "Micron"},
	{NAND_MFR_AMD, "AMD"},
	{0x0, "Unknown"}
};

/*------------------------------------------------------------------*/
static inline unsigned long rd_regl(unsigned offset)
{
	return *REG32(P4A_NANDC_PHYS + offset);
}

static inline void wr_regl(unsigned offset, unsigned val)
{
	*REG32(P4A_NANDC_PHYS + offset) = val;
}

static inline void nfc_clearfifo()
{
	wr_regl(NFC_RDATA, NRDR_DATA_VALID);
}

/*
 * status check
 */
typedef uint8_t wait_mode_t;
typedef uint32_t wait_cond_t;

#define WAITMODE_AND		((wait_mode_t)0)	// all bits must be set
#define WAITMODE_OR			((wait_mode_t)1)	// any bit must be set

static int wait_condition(wait_cond_t cond, wait_mode_t	mode, int timeout)
{
    int ret = 0;
    unsigned long nsr;

    while (timeout--) {
        nsr = rd_regl(NFC_STATUS);

        switch(mode) {
            case WAITMODE_OR:
            {
                if(cond & nsr)
                    goto cond_meet;
                break;
            }
            case WAITMODE_AND:
            default:
            {
                if((cond & nsr) == cond)
                    goto cond_meet;
                break;
            }
        }
        udelay(1);
    }

    PRINT("wait 0x%x in NSR to be set timeout!\n", cond);

    ret = -1;
cond_meet:
    return ret;
}


#define wait_cmdQ_done()		wait_condition(NSR_CMDQ_DONE, WAITMODE_AND, 4000)
#define wait_device_ready()		wait_condition(NSR_DEV_RDY, WAITMODE_AND, 4000)

typedef struct nfc_cmd_entry {
	union {
		u32 value;
		struct {
			u32	cmd_param:8;
			u32	cmd_type:4;
			u32	reserved:20;

		};
	};
}nfc_cmd_entry_t;

static int nfc_send_cmdQ(struct nfc_cmd_entry* entry, int num)
{
	int i;
	int ret = 0;

	// push the command input command queue
	for(i=0; i<num; i++) {
		wr_regl(NFC_CMDQ_ENTRY, entry[i].value);
	}
	
	// set the command queue counter, and start execute it.
	wr_regl(NFC_CMDQ_CTRL, NCQCR_SET_LOOP(1));
	
	ret = wait_cmdQ_done();
	if (ret) {
		PRINT("wait command queue done timeout!\n");
	}
	
	return ret;
}

/* align x on a size boundary - adjust x up/down if needed */
#define _ALIGN_UP(x, size)    (((x)+((size)-1)) & (~((size)-1)))
#define _ALIGN_DOWN(x, size)  ((x) & (~((size)-1)))

static inline uint32_t block_aligned_page(uint32_t page)
{
	return _ALIGN_DOWN(page, chip->pages_per_block);
}

static inline int is_first_page_of_block(uint32_t page)
{
	return (block_aligned_page(page) == page);
}

static inline int is_large_page()
{
	return (chip->info->pagesize > 512);
}

#define ASSERT_CS	\
	do {	\
		chip_select(0);	\
	} while(0)

#define DEASSERT_CS	\
	do {	\
		chip_select(-1);	\
	} while (0)

static uint8_t nand_read_byte();

void chip_select(int chip)
{
	if (chip != -1)
		wr_regl(NFC_CE, (chip << 1));
	else
		wr_regl(NFC_CE, 1);
}


static int read_id(uint8_t *buff, int len)
{
	uint8_t	IDs[8];
	int i;
    
	struct nfc_cmd_entry cmds[10];
	int idx;
	int ret;



	PRINT("read ID\n");

	memset(cmds, 0, sizeof(struct nfc_cmd_entry) * 10);
	
	idx = 0;
	cmds[idx].cmd_param = NAND_CMD_READID;
	cmds[idx++].cmd_type = CMDQ_SET_CMD;

	cmds[idx].cmd_param = 0;
	cmds[idx++].cmd_type = CMDQ_SET_ADDR;
	
	cmds[idx++].cmd_type = CMDQ_SINGLE_RD;
	cmds[idx++].cmd_type = CMDQ_SINGLE_RD;
	cmds[idx++].cmd_type = CMDQ_SINGLE_RD;
	cmds[idx++].cmd_type = CMDQ_SINGLE_RD;
	cmds[idx++].cmd_type = CMDQ_SINGLE_RD;
	cmds[idx++].cmd_type = CMDQ_SINGLE_RD;
	cmds[idx++].cmd_type = CMDQ_END_QUEUE;

	nfc_clearfifo();
	
	ret = nfc_send_cmdQ(&cmds[0], idx);
	

	if(ret){
		PRINT("read ID failed\n");
		return ret;
	}

	for(i = 0; i < MIN(len, 8); i++)
	{
		buff[i] = nand_read_byte();
	}

	return 0;
}
static int nand_read_status()
{
	struct nfc_cmd_entry cmds[5];
	int idx = 0;
	int ret;

	PRINT("read status!\n");

	memset(cmds, 0, sizeof(struct nfc_cmd_entry) * 5);
	
	cmds[idx].cmd_param = NAND_CMD_STATUS;
	cmds[idx++].cmd_type = CMDQ_SET_CMD;
	
	cmds[idx++].cmd_type = CMDQ_SINGLE_RD;
	cmds[idx++].cmd_type = CMDQ_END_QUEUE;

	nfc_clearfifo();

	ret = nfc_send_cmdQ(&cmds[0], idx);

	return ret;
}

static int nand_reset()
{
	struct nfc_cmd_entry cmds[5];
	int idx = 0;
	int ret;
	
	PRINT("nand reset!\n");
	memset(cmds, 0, sizeof(struct nfc_cmd_entry) * 5);
	
	cmds[idx].cmd_param = NAND_CMD_RESET;
	cmds[idx++].cmd_type = CMDQ_SET_CMD;
	
	cmds[idx++].cmd_type = CMDQ_WAIT_RDY;
	
	cmds[idx++].cmd_type = CMDQ_END_QUEUE;
	
	ret = nfc_send_cmdQ(&cmds[0], idx);

	return ret;
}

static int nfc_erase_block(int page_addr)
{
	int isLargePage = is_large_page();
	struct nfc_cmd_entry cmds[10];
	int idx = 0;
	int ret;

	PRINT("erase block (page_addr = 0x%x)\n", page_addr);

	memset(cmds, 0, sizeof(struct nfc_cmd_entry) * 10);

	cmds[idx].cmd_param = NAND_CMD_ERASE1;
	cmds[idx++].cmd_type = CMDQ_SET_CMD;

	cmds[idx].cmd_param = page_addr & 0xFF;
	cmds[idx++].cmd_type = CMDQ_SET_ADDR;
	
	cmds[idx].cmd_param = (page_addr >> 8) & 0xFF;
	cmds[idx++].cmd_type = CMDQ_SET_ADDR;
	
	if ((!isLargePage && (chip->info->pagesize * chip->info->chips) > 0x2000000) || (isLargePage && (chip->info->pagesize * chip->info->chips) >= 0x10000000)) {
		// small page, device size greater than 32MByte
		// or large page, device size equal or greater than 256MByte
		cmds[idx].cmd_param = (page_addr >> 16) & 0xFF;
		cmds[idx++].cmd_type = CMDQ_SET_ADDR;
	}
	
	cmds[idx].cmd_param = NAND_CMD_ERASE2;
	cmds[idx++].cmd_type = CMDQ_SET_CMD;
		
	cmds[idx++].cmd_type = CMDQ_WAIT_RDY;
	cmds[idx++].cmd_type = CMDQ_CHECK_STATUS;
	
	cmds[idx++].cmd_type = CMDQ_END_QUEUE;
	
	ret = nfc_send_cmdQ(&cmds[0], idx);
	
	return 0;
}

static int nfc_page_program(int column, int page_addr)
{
	int isLargePage = is_large_page();
	struct nfc_cmd_entry cmds[15];
	int idx = 0;
	int ret;

	if(column != 0 && column != chip->info->pagesize)
	{
		PRINT("nfc_page_program invalid param column%d chip->info->pagesize%d", column, chip->info->pagesize);
		return -1;
	}
	PRINT("page program (page_addr=0x%x, column=%d)\n", page_addr, column);
	
	memset(cmds, 0, sizeof(struct nfc_cmd_entry) * 15);

	if(!isLargePage) {
		if(column >= 512)	//access spare area
			cmds[idx].cmd_param = NAND_CMD_READOOB;
		else
			cmds[idx].cmd_param = NAND_CMD_READ0;
		cmds[idx++].cmd_type = CMDQ_SET_CMD;
	}

	cmds[idx].cmd_param = NAND_CMD_SEQIN;
	cmds[idx++].cmd_type = CMDQ_SET_CMD;

	// addressing column
	cmds[idx].cmd_param = column & 0xFF;
	cmds[idx++].cmd_type = CMDQ_SET_ADDR;	
	if (isLargePage) {	
		cmds[idx].cmd_param = (column >>8) & 0xFF;
		cmds[idx++].cmd_type = CMDQ_SET_ADDR;
	}
	
	// addressing row
	cmds[idx].cmd_param = page_addr & 0xFF;
	cmds[idx++].cmd_type = CMDQ_SET_ADDR;
	
	cmds[idx].cmd_param = (page_addr >> 8) & 0xFF;
	cmds[idx++].cmd_type = CMDQ_SET_ADDR;

	if ((!isLargePage && (chip->info->pagesize * chip->info->chips) > 0x2000000) || (isLargePage && (chip->info->pagesize * chip->info->chips) >= 0x10000000)) {
		// small page, device size greater than 32MByte
		// or large page, device size equal or greater than 256MByte
		cmds[idx].cmd_param = (page_addr >> 16) & 0xFF;
		cmds[idx++].cmd_type = CMDQ_SET_ADDR;
	}

	chip->wOffset = 0;
	if (isLargePage) {
		if (column >= 2048) {	
			// write spare area
			cmds[idx].cmd_param = (CMD_DMA_ECC_OFF | CMD_DMA_64BYTES | CMD_DMA_WRITE) ;
			cmds[idx++].cmd_type = CMDQ_RUN_DMA;
			chip->wOffset = 2048;
		}else {	
			cmds[idx].cmd_param = (CMD_DMA_ECC_OFF | CMD_DMA_2048BYTES | CMD_DMA_WRITE) ;
			cmds[idx++].cmd_type = CMDQ_RUN_DMA;
			cmds[idx].cmd_param = (CMD_DMA_ECC_OFF | CMD_DMA_64BYTES | CMD_DMA_WRITE) ;
			cmds[idx++].cmd_type = CMDQ_RUN_DMA;
		}
	
	}else {
		if (column >= 512) {
			//write spare area
			cmds[idx].cmd_param = (CMD_DMA_ECC_OFF | CMD_DMA_16BYTES | CMD_DMA_WRITE) ;
			cmds[idx++].cmd_type = CMDQ_RUN_DMA;
			chip->wOffset = 512;
		}else {
			cmds[idx].cmd_param = (CMD_DMA_ECC_OFF | CMD_DMA_512BYTES | CMD_DMA_WRITE) ;
			cmds[idx++].cmd_type = CMDQ_RUN_DMA;
			cmds[idx].cmd_param = (CMD_DMA_ECC_OFF | CMD_DMA_16BYTES | CMD_DMA_WRITE) ;
			cmds[idx++].cmd_type = CMDQ_RUN_DMA;
		}
	
	}

	cmds[idx].cmd_param = NAND_CMD_PAGEPROG;
	cmds[idx++].cmd_type = CMDQ_SET_CMD;

	cmds[idx++].cmd_type = CMDQ_WAIT_RDY;
	
	cmds[idx++].cmd_type = CMDQ_CHECK_STATUS;
	
	cmds[idx++].cmd_type = CMDQ_END_QUEUE;
	
	/* set dma start address */
	wr_regl(NFC_DMA_ADDR, NAND_BUFFER_PHY_ADDRESS + chip->wOffset);

	ret = nfc_send_cmdQ(&cmds[0], idx);

	PRINT("NSR = 0x%08x\n", rd_regl(NFC_STATUS));

	return ret;
}

static int nfc_page_read(int column, int page_addr)
{
	int isLargePage = is_large_page();
	struct nfc_cmd_entry cmds[15];
	int idx;
	int ret;

	chip->rOffset = column;
	
	if(column != 0 && column != chip->info->pagesize)
	{
		PRINT("nfc_page_read invalid param column%d chip->info->pagesize%d", column, chip->info->pagesize);
		return -1;
	}

	PRINT("read page 0x%x, with column=%d\n", page_addr, column);

	memset(cmds, 0, sizeof(struct nfc_cmd_entry) * 15);

	idx = 0;
	if(!isLargePage && column >= 512)
		cmds[idx].cmd_param = NAND_CMD_READOOB;
	else
		cmds[idx].cmd_param = NAND_CMD_READ0;
	cmds[idx++].cmd_type = CMDQ_SET_CMD;
	
	// column address cycle(s)
	cmds[idx].cmd_param = column & 0xFF;
	cmds[idx++].cmd_type = CMDQ_SET_ADDR;	
	if(isLargePage) {
		cmds[idx].cmd_param = (column >> 8) & 0xFF;
		cmds[idx++].cmd_type = CMDQ_SET_ADDR;
	}

	// row address cycles
	cmds[idx].cmd_param = page_addr & 0xFF;
	cmds[idx++].cmd_type = CMDQ_SET_ADDR;
	
	cmds[idx].cmd_param = (page_addr >> 8) & 0xFF;
	cmds[idx++].cmd_type = CMDQ_SET_ADDR;
	
	if ((!isLargePage && (chip->info->pagesize * chip->info->chips) > 0x2000000) || (isLargePage && (chip->info->pagesize * chip->info->chips) >= 0x10000000)) {
		// small page, device size greater than 32MByte
		// or large page, device size equal or greater than 256MByte
		cmds[idx].cmd_param = (page_addr >> 16) & 0xFF;
		cmds[idx++].cmd_type = CMDQ_SET_ADDR;
	}
	
	if (isLargePage) {
		cmds[idx].cmd_param = NAND_CMD_READSTART;
		cmds[idx++].cmd_type = CMDQ_SET_CMD;
	}
	
	cmds[idx++].cmd_type = CMDQ_WAIT_RDY;

	if (isLargePage) {
		if (column >= 2048) {
			cmds[idx].cmd_param = (CMD_DMA_ECC_OFF | CMD_DMA_64BYTES |CMD_DMA_READ) ;
			cmds[idx++].cmd_type = CMDQ_RUN_DMA;
			chip->rOffset -= 2048;

		}else {
			cmds[idx].cmd_param = (CMD_DMA_ECC_OFF | CMD_DMA_2048BYTES | CMD_DMA_READ) ;
			cmds[idx++].cmd_type = CMDQ_RUN_DMA;
		
			cmds[idx].cmd_param = (CMD_DMA_ECC_OFF | CMD_DMA_64BYTES | CMD_DMA_READ) ;
			cmds[idx++].cmd_type = CMDQ_RUN_DMA;
		}

	} else {
		if (column >= 512) {
			cmds[idx].cmd_param = (CMD_DMA_ECC_OFF | CMD_DMA_16BYTES |CMD_DMA_READ) ;
			cmds[idx++].cmd_type = CMDQ_RUN_DMA;
			chip->rOffset -= 512;

		}else {
			cmds[idx].cmd_param = (CMD_DMA_ECC_OFF | CMD_DMA_512BYTES | CMD_DMA_READ) ;
			cmds[idx++].cmd_type = CMDQ_RUN_DMA;
		
			cmds[idx].cmd_param = (CMD_DMA_ECC_OFF | CMD_DMA_16BYTES | CMD_DMA_READ) ;
			cmds[idx++].cmd_type = CMDQ_RUN_DMA;
		}

	}

	cmds[idx++].cmd_type = CMDQ_END_QUEUE;
	
	/* set dma start address */
	wr_regl(NFC_DMA_ADDR, NAND_BUFFER_PHY_ADDRESS);
		
	ret = nfc_send_cmdQ(&cmds[0], idx);
	
	return ret;
}

static uint8_t nand_read_byte()
{
	u32 data;

	data = rd_regl(NFC_RDATA);
	PRINT("read byte 0x%x\n", data);

	/* if data valid then return valid data, otherwise return 0. */
	return (data & NRDR_DATA_VALID) ? (data & NRDR_DATA_MASK) : 0;
}

static void nand_write_buf(const uint8_t *buf, int len)
{
	
	memcpy((void*)(NAND_BUFFER_PHY_ADDRESS + chip->wOffset), buf, len);
	chip->wOffset += len;
}


static void nand_read_buf(uint8_t *buf, int len)
{
	memcpy(buf, (void*)(NAND_BUFFER_PHY_ADDRESS + chip->rOffset), len);
	chip->rOffset += len;
}

static int nand_verify_buf(const uint8_t *buf, int len)
{
	int i, offset=chip->rOffset;

	for (i=0; i<len; i++)
		if (buf[i] != ((uint8_t*)NAND_BUFFER_PHY_ADDRESS)[offset + i])
			return -1;

	chip->rOffset += len;
	return 0;
}

static int p4a_nand_dev_ready()
{
	return 1;
}


// wait for command done, applies to erase and program
// return nand status register value
static int nand_waitfunc()
{
	int status;
	
	if (NRET_NOERR != wait_device_ready()) {
		PRINT("wait device ready timeout!\n");
	}
	
	status = (rd_regl(NFC_STATUS) & NSR_CUR_STATUS_MASK) >> NSR_CUR_STATUS_SHIFT;

	PRINT("waitfunc: status=0x%x\n", status);

	return status;
}

static int erase_block(uint32_t page)
{
	int status = 0;
	if (!is_first_page_of_block(page))
		return -1;

	nfc_erase_block(page);

	status = nand_waitfunc();
	/* See if block erase succeeded */
	if (status & NAND_STATUS_FAIL) {
		PRINT("%s: Failed erase, "
				"page 0x%08x\n", __func__, page);
		return -1;
	}

	return 0;
}

static int program_page(uint32_t page, uint8_t *buff, size_t len, int extra_per_page)
{
	struct nandflash_info* info = chip->info;
	uint32_t column = 0;
	size_t darea_len = 0;
	size_t sarea_len = 0;
	int status = 0;

	if (!info)
		return -1;

	darea_len = MIN(len, info->pagesize);
	if (extra_per_page) {
		sarea_len = MIN(info->oobsize, len-darea_len);
	}

	PRINT("write_page: darea_len = %d, sarea_len = %d, hwecc=%d\n", darea_len, sarea_len, chip->hwecc);

	memset((void*)NAND_BUFFER_PHY_ADDRESS, 0xFF, MAX_PAGECACHE_SIZE);	
	/* write data area data */
	if (darea_len > 0) {
		nand_write_buf(buff, darea_len);
	}

	/* write spare area data */
	if (sarea_len > 0) {
		nand_write_buf(buff + darea_len, sarea_len);
	}

	nfc_page_program(column, page);

	status = nand_waitfunc();
	if (status & NAND_STATUS_FAIL){
		PRINT("page program failed\n");
		return -1;
	}

	return 0;
}

static int read_page(uint32_t page, uint8_t *buff, size_t len, int extra_per_page)
{
	struct nandflash_info* info = chip->info;
	uint32_t column = 0;
	int i, addr_i = 0;
	uint32_t wait_event_mask;
	uint32_t avail = 0;
	uint8_t *ptr;
	size_t darea_len = 0;
	size_t sarea_len = 0;

	if (!info)
		return -1;

	darea_len = MIN(len, info->pagesize);
	if (extra_per_page){
		sarea_len = MIN(info->oobsize, len-darea_len);
	}

	PRINT("read_page: darea_len = %d, sarea_len = %d, hwecc=%d\n", darea_len, sarea_len, chip->hwecc);

	if ((darea_len + sarea_len) < len) {
		memset(&buff[darea_len + sarea_len], 0xff, len - darea_len - sarea_len);
	}

	if (darea_len > 0) {
		nfc_page_read(column, page);
		nand_read_buf(buff, darea_len);
	}

	if (sarea_len > 0) {
		column = info->pagesize;
		nfc_page_read(column, page);
		nand_read_buf(buff + darea_len, sarea_len);
	}


	return 0;
}

static int read_oob(uint32_t page, uint8_t *buff, size_t len)
{
	struct nandflash_info* info = chip->info;
	uint32_t column;
	int i, addr_i =0;
	uint8_t *ptr;
	size_t sarea_len;

	if (!info)
		return -1;

	//sarea_len = MIN(len, chip->freeoob_bytes);
	sarea_len = MIN(len, info->oobsize);
	if (sarea_len < len) {
		memset(&buff[sarea_len], 0xff, len - sarea_len);
	}
	
	if (sarea_len > 0) {
		column = info->pagesize;
		nfc_page_read(column, page);
		nand_read_buf(buff, sarea_len);
	}
	
	return 0;
}

/**
 * write_oob-  OOB data write function
 * @page:	page number to write
 * @buf: buffer contained the data to be write
 * @len: len of the data to be wrote to oob 
 * @ooboffs:	offset start of oob
 
 */
static int write_oob(int page, uint8_t *buf, size_t len, uint32_t ooboffs )
{
	int status = 0;
	int column;

	if(len > chip->info->oobsize){
		PRINT("Invalid oobuflen %d oobsize%d", len, chip->info->oobsize);
		return -1;
	}
	
	if(page > (chip->info->chipsize / chip->info->pagesize)){
		PRINT("Invalid page%d chipsize%d pagsize%sd", page, chip->info->chipsize, chip->info->pagesize);
		return -1;
	}
	
	column = chip->info->pagesize;
	memset((void *)NAND_BUFFER_PHY_ADDRESS, 0xFF, NAND_MAX_OOBSIZE);	
	chip->wOffset += chip->info->pagesize + ooboffs;
	nand_write_buf(buf, len);
	nfc_page_program(column, page);
	
	status = nand_waitfunc();
	if (status & NAND_STATUS_FAIL)
	{
		PRINT("page program oob failed\n");
		return -1;
	}

	return status;
}


/* if return 1, indicates a bad block */
static int check_bad_block(uint32_t page_addr)
{
	struct nandflash_info* info = chip->info;
	int column; 
	int i, addr_i = 0;
	uint8_t data;

	return 0;
}


/*----------------Nand Flash APIs----------------------*/
int nand_flash_readid(uint8_t *IDs, int num)
{
	int status = 0; 
	chip_select(0);
	status = read_id(IDs, num);
	chip_select(-1);

	return status;
}

static unsigned long long lludiv(unsigned long long a, unsigned long long b)
{
	unsigned long long ret = 0;
	if(b == 0)
	{
		PRINT("fatal error divisor is 0 \n");
		return -1;
	}
	while(a >= b)
	{
		a -= b;
		ret++;
	}
	return ret;
}

#define hweight8(w)		\
      ((!!((w) & (1ULL << 0))) +	\
	(!!((w) & (1ULL << 1))) +	\
	(!!((w) & (1ULL << 2))) +	\
	(!!((w) & (1ULL << 3))) +	\
	(!!((w) & (1ULL << 4))) +	\
	(!!((w) & (1ULL << 5))) +	\
	(!!((w) & (1ULL << 6))) +	\
	(!!((w) & (1ULL << 7)))	)

/**
 * nand_block_bad - [DEFAULT] Read bad block marker from the chip
 * @ofs:	offset from device start
 * @getchip:	0, if the chip is already selected
 *
 * Check, if the block is bad.
 */
static int nand_block_bad(loff_t ofs, int getchip)
{
	int page, chipnr, res = 0;
	u8 buff[NAND_SMALL_BADBLOCK_POS + 1];
	u16 bad;

	if (chip->options & NAND_BBT_SCANLASTPAGE)
		ofs += chip->info->blocksize- chip->info->pagesize;

	page = (int)(ofs >> chip->page_shift) & chip->pagemask;

	if (getchip) {
		chipnr = (int)(ofs >> chip->chip_shift);

		/* Select the NAND device */
		chip_select(chipnr);
	}

	read_oob(page, buff, NAND_SMALL_BADBLOCK_POS + 1);

	bad =  buff[chip->badblockpos];
	
	if (chip->badblockbits == 8)
		res = bad != 0xFF;
	else
		res = hweight8(bad) < chip->badblockbits;

	if (getchip)
		chip_select(-1);

	return res;
}

/**
 * nand_default_block_markbad - mark a block bad
 * @ofs:	offset from device start
 *
*/
int nand_block_markbad(loff_t ofs)
{
	uint8_t buf[2] = { 0, 0 };
	int block, ret, i = 0;
	int chipnr;
	int page;

	if (chip->options & NAND_BBT_SCANLASTPAGE)
		ofs += chip->info->blocksize- chip->info->pagesize;

	/* Get block number */
	block = (int)(ofs >> chip->bbt_erase_shift);

	chipnr = (int)(ofs >> chip->chip_shift);

	/* Select the NAND device */
	chip_select(chipnr);

	/*
	 * Reset the chip. Some chips (like the Toshiba TC5832DC found in one
	 * of my DiskOnChip 2000 test units) will clear the whole data page too
	 * if we don't do this. I have no clue why, but I seem to have 'fixed'
	 * it in the doc2000 driver in August 1999.  dwmw2.
	 */
	 nand_reset();

	/* Write to first two pages and to byte 1 and 6 if necessary.
	 * If we write to more than one location, the first error
	 * encountered quits the procedure. We write two bytes per
	 * location, so we dont have to mess with 16 bit access.
	 */
	 
	do {
		uint32_t ooboffs;
		/* Shift to get page */
		page = (int)(ofs >> chip->page_shift);
	
		ooboffs = chip->badblockpos & ~0x01;
		
		ret = write_oob(page, buf, 2, ooboffs);

		if (!ret && (chip->options & NAND_BBT_SCANBYTE1AND6)) {
			ooboffs = NAND_SMALL_BADBLOCK_POS
				& ~0x01;
			ret = write_oob(page, buf, 2, ooboffs);
		}
		i++;
		ofs += chip->info->pagesize;
	} while (!ret && (chip->options & NAND_BBT_SCAN2NDPAGE) &&
			i < 2);

	chip_select(-1);

	return ret;
}



int nand_flash_erase(struct erase_info* ei)
{
	struct nandflash_info *info = chip->info;
	int status = 0;
	unsigned page, block;

	unsigned interval;

	int chipnr;

	if (!info)
		return -1;
	
	chipnr = ei->from >> chip->chip_shift;

	ei->done = ei->fail = 0;
	ei->total = lludiv(_ALIGN_UP(ei->len, (lsize_t)info->blocksize) ,(lsize_t)info->blocksize);
	interval = MIN(ei->total, 10);

	block = lludiv(ei->from, info->blocksize);	/* start block address */

	chip_select(chipnr);
	while ((ei->done + ei->fail) < ei->total) {
		page = block * chip->pages_per_block;
		status = erase_block(page);

		if (status == 0) {
			ei->done++;

			if (ei->callback && (ei->done % interval) == 0) {
				ei->progress = (ei->done + ei->fail) * 100 / (ei->total);
				ei->callback(ei, ERASE_EVENT_PROGRESS);
			}
		} else {
			ei->fail++;

			if (ei->callback) {
				ei->fail_addr = page << chip->page_shift;
				ei->callback(ei, ERASE_EVENT_FAIL);
			}
		}

		block++;
	}
	chip_select(-1);

	if (ei->callback && ei->progress != 100) {
		ei->progress = 100;
		ei->callback(ei, ERASE_EVENT_PROGRESS);
	}

	return status;
}

int nand_flash_read(struct read_info *ri)
{
	struct nandflash_info *info = chip->info;
	unsigned page;	// start page
	unsigned real_pagesize;

	unsigned total, done, interval;
	int chipnr;

	if (!info)
		return -1;

	/* shift to get first page */
	page = (ri->from >> chip->page_shift) & chip->pagemask;
	chipnr = ri->from >> chip->chip_shift;

	real_pagesize = ri->withoob ? (info->pagesize + info->oobsize) : info->pagesize;

	total = _ALIGN_UP(ri->len, real_pagesize) / real_pagesize;
	done = 0;
	interval = MIN(total, 10);

	ri->retlen = 0;

	chip_select(chipnr);

	while (done < total) {
		if (is_first_page_of_block(page)) {
			if(nand_block_bad(ri->from, 0)){
				PRINT("block %d is bad\n", page/(info->blocksize/info->pagesize));
				page += chip->pages_per_block;
				continue;
			}
		}

		read_page(page, (uint8_t*)ri->buf + ri->retlen, real_pagesize, ri->withoob);

		ri->retlen += real_pagesize;
		done++;
		page++;

		if (ri->callback && (done % interval) == 0) {
			ri->callback(ri, done, total);
		}
	}

	chip_select(-1);

	if (ri->callback)
		ri->callback(ri, done, total);

	return 0;
}


int nand_flash_readoob(uint8_t *buff, unsigned long offset, size_t len)
{
	struct nandflash_info* info = chip->info;
	int chipnr;
	unsigned page;

	if (!info)
		return -1;
	
	page = (offset >> chip->page_shift) & chip->pagemask;
	chipnr = (offset >> chip->chip_shift);

	chip_select(chipnr);
	
	read_oob(page, buff, len);

	chip_select(-1);

	return 0;
}

int nand_flash_write(struct program_info* pi)
{
	struct nandflash_info* info = chip->info;
	int status = 0;
	unsigned page;	/* start page address */
	unsigned real_pagesize; 

	unsigned done, total, interval;
	int chipnr;

	if (!info)
		return -1;

	real_pagesize = pi->withoob ? (info->pagesize + info->oobsize) : info->pagesize;

	total = _ALIGN_UP(pi->len, real_pagesize) / real_pagesize;
	done = 0;
	interval = MIN(10, total);

	page = (pi->from >> chip->page_shift) & chip->pagemask;
	chipnr = (pi->from >> chip->chip_shift);
	pi->retlen = 0;

	chip_select(chipnr);
	while (1) {
		if (pi->retlen >= pi->len)
			break;

		if (is_first_page_of_block(page)) {
			if(nand_block_bad(pi->from, 0)){
				PRINT("block %d is bad", page /(info->blocksize/info->pagesize));
				page += chip->pages_per_block;
				continue;
			}
		}

		status = program_page(page, (uint8_t*)pi->buf + pi->retlen, real_pagesize, pi->withoob);
		if (status == 0) {
			pi->retlen += real_pagesize;
			done++;
			if (pi->callback && (done % interval == 0)) {
				pi->callback(pi, done, total);
			}
		}

		page++;
	}
	chip_select(-1);

	if (pi->callback)
		pi->callback(pi,done, total);
	
	return status;
}

int nand_flash_hwecc(int on)
{
	struct nandflash_info* info = chip->info;

	if (!info)
		return -1;

	if (on) {
		chip->hwecc = HWECC_8BITS;

		chip->sector_size = 512;
		chip->hwecc_bytes = info->buswidth16 ? 14 : 13;
		chip->freeoob_bytes = info->oobsize - ((info->pagesize / chip->sector_size) * chip->hwecc_bytes);

	} else {
		chip->hwecc = HWECC_OFF;

		chip->sector_size = 512;
		chip->hwecc_bytes = 0;
		chip->freeoob_bytes = info->oobsize;
	}

	return 0;
}

struct nandflash_info* nand_flash_get_flashinfo(void)
{
	if(NULL == chip){
		return NULL;
	}
	else{
		return chip->info;
	}
}

int nand_flash_set_flashinfo(struct nandflash_info* _info)
{
	struct nandflash_info *info = chip->info;
	int update = 0;

	if (!info) {
		static struct nandflash_info nf_info;
		info = &nf_info;
		update = 1;

	} else if (memcmp((void*)info, (void*)_info, 5*sizeof(uint32_t))) {	//diff
		update = 1;
	}

	if (!update)
		return 0;

	if (_info->pagesize != 512 && _info->pagesize != 2048 && _info->pagesize != 4096) {
		PRINT("nand pagesize %d not support!\n", _info->pagesize);
		return -1;
	}

	if (_info->blocksize < KiB(1) || (_info->blocksize % _info->pagesize) != 0) {
		PRINT("nand blocksize %d invalid!\n", _info->blocksize);
		return -1;
	}

	if ((_info->chipsize << 10) % (_info->blocksize >> 10) != 0) {
		PRINT("nand chipsize %d invalid!\n", _info->blocksize);
		return -1;
	}

	info->pagesize = _info->pagesize;
	info->blocksize = _info->blocksize;
	info->chipsize = _info->chipsize;

	info->buswidth16 = _info->buswidth16;
	info->chips = _info->chips;
	info->oobsize = (info->pagesize >> 9) * 16;

	if (!chip->info)
		chip->info = info;

	chip->page_shift = generic_ffs(info->pagesize) - 1;
	chip->block_shift = generic_ffs(info->blocksize) - 1;
	chip->chip_shift = 20 + (generic_ffs(info->chipsize) - 1);

	/* convert chipsize to number of pages per chip - 1 */
	chip->pagemask = (info->chipsize * 1024 / info->pagesize * 1024) - 1;

	chip->pages_per_block = info->blocksize / info->pagesize;

	chip->row_cycles = 2;
	if (info->pagesize > 1024) {
		unsigned int div = info->chipsize / (info->pagesize >> 11);

		chip->column_cycles = 2;

		/* 
		 * Notice: nand->chipsize unit is MiB 
		 * for 2k page size Nand, chip size greater than 128MiB, there will 3 row cycles.
		 * for 4k page size Nand, chip size greater than 256MiB, there will 3 row cycles.
		 * for 8k page size Nand, chip size greater than 512MiB, there will 3 row cycles.
		 */
		if (div > 128)
			chip->row_cycles++;

		/* 
		 * Notice: nand->chipsize unit is MiB
		 * for 2k page size Nand, chip size greater than 32GB, there will 4 row cycles.
		 * for 4k page size Nand, chip size greater than 64GB, there will 4 row cycles.
		 * for 8k page size Nand, chip size greater than 128GB, there will 4 row cycles.
		 */
		if (div > KiB(32))
			chip->row_cycles++;	

	} else {
		chip->column_cycles = 1;

		/* 
		 * Notice: nand->chipsize unit is MiB
		 */
		if (info->chipsize > 32)
			chip->row_cycles++;
	}



	nand_flash_hwecc(chip->hwecc);

	return 0;
}
static struct nandflash_info* detect_nand_flash(void)
{
	struct nandflash_info* flash_info;

	if(nand_scan_ident(1, NULL)){
		PRINT("nand scan failed unknown nand");
		return NULL;
	}

	flash_info = nand_flash_get_flashinfo();

	return flash_info;
}

/*
 * Get the flash and manufacturer id and lookup if the type is supported
 */
static struct nand_flash_dev *nand_get_flash_type(int busw,
						  int *maf_id, int *dev_id,
						  struct nand_flash_dev *type)
{
	int i, maf_idx;
	u8 id_data[8];
	int ret;
	
	/* Select the device */
	chip_select(0);
	/*
	 * Reset the chip, required by some chips (e.g. Micron MT29FxGxxxxx)
	 * after power-up
	 */
	nand_reset();

	/* Send the command for reading device ID */
	if(read_id(id_data, 8)){
		return NULL;
	}

	/* Read manufacturer and device IDs */
	*maf_id = id_data[0];
	*dev_id = id_data[1];

	/* Try again to make sure, as some systems the bus-hold or other
	 * interface concerns can cause random data which looks like a
	 * possibly credible NAND flash to appear. If the two results do
	 * not match, ignore the device completely.
	 */

	if(read_id(id_data, 8)){
		return NULL;
	}

	if (id_data[0] != *maf_id || id_data[1] != *dev_id) {
		PRINT("%s: second ID read did not match "
		       "%02x,%02x against %02x,%02x\n", __func__,
		       *maf_id, *dev_id, id_data[0], id_data[1]);
		return NULL;
	}

	if (!type)
		type = nand_flash_ids;

	for (; type->name != NULL; type++)
		if (*dev_id == type->id)
			break;

	if (!type->name){
		PRINT("flash type name NULL");
		return NULL;
	}

	chip->info->chipsize = type->chipsize;

	if(!type->pagesize) {
		int extid;
		/* The 3rd id byte holds MLC / multichip data */
		chip->cellinfo = id_data[2];
		/* The 4th id byte is the important one */
		extid = id_data[3];

		/*
		 * Field definitions are in the following datasheets:
		 * Old style (4,5 byte ID): Samsung K9GAG08U0M (p.32)
		 * New style   (6 byte ID): Samsung K9GBG08U0M (p.40)
		 *
		 * Check for wraparound + Samsung ID + nonzero 6th byte
		 * to decide what to do.
		 */
		if (id_data[0] == id_data[6] && id_data[1] == id_data[7] &&
				id_data[0] == NAND_MFR_SAMSUNG &&
				(chip->cellinfo & NAND_CI_CELLTYPE_MSK) &&
				id_data[5] != 0x00) {
			/* Calc pagesize */
			chip->info->pagesize = 2048 << (extid & 0x03);
			extid >>= 2;
			/* Calc oobsize */
			switch (extid & 0x03) {
			case 1:
				chip->info->oobsize = 128;
				break;
			case 2:
				chip->info->oobsize = 218;
				break;
			case 3:
				chip->info->oobsize = 400;
				break;
			default:
				chip->info->oobsize = 436;
				break;
			}
			extid >>= 2;
			/* Calc blocksize */
			chip->info->blocksize= (128 * 1024) <<
				(((extid >> 1) & 0x04) | (extid & 0x03));
			chip->info->buswidth16= 0;
		} else {
			/* Calc pagesize */
			chip->info->pagesize = 1024 << (extid & 0x03);
			extid >>= 2;
			/* Calc oobsize */
			chip->info->oobsize = (8 << (extid & 0x01)) *
				(chip->info->pagesize>> 9);
			extid >>= 2;
			/* Calc blocksize. Blocksize is multiples of 64KiB */
			chip->info->blocksize= (64 * 1024) << (extid & 0x03);
			extid >>= 2;
			/* Get buswidth information */
			busw= (extid & 0x01) ? NAND_BUSWIDTH_16 : 0;
		}
	} else {
		/*
		 * Old devices have chip data hardcoded in the device id table
		 */
		chip->info->blocksize= type->erasesize;
		chip->info->pagesize = type->pagesize;
		chip->info->oobsize = chip->info->pagesize/ 32;
		busw = type->options & NAND_BUSWIDTH_16;

		/*
		 * Check for Spansion/AMD ID + repeating 5th, 6th byte since
		 * some Spansion chips have erasesize that conflicts with size
		 * listed in nand_ids table
		 * Data sheet (5 byte ID): Spansion S30ML-P ORNAND (p.39)
		 */
		if (*maf_id == NAND_MFR_AMD && id_data[4] != 0x00 &&
				id_data[5] == 0x00 && id_data[6] == 0x00 &&
				id_data[7] == 0x00 &&chip->info->pagesize == 512) {
			chip->info->blocksize= 128 * 1024;
			chip->info->blocksize <<= ((id_data[3] & 0x03) << 1);
		}
	}
	/* Get chip options, preserve non chip based options */
	chip->options &= ~NAND_CHIPOPTIONS_MSK;
	chip->options |= type->options & NAND_CHIPOPTIONS_MSK;

	/* Check if chip is a not a samsung device. Do not clear the
	 * options for chips which are not having an extended id.
	 */
	if (*maf_id != NAND_MFR_SAMSUNG && !type->pagesize)
		chip->options &= ~NAND_SAMSUNG_LP_OPTIONS;
ident_done:

	/*
	 * Set chip as a default. Board drivers can override it, if necessary
	 */
	chip->options |= NAND_NO_AUTOINCR;

	/* Try to identify manufacturer */
	for (maf_idx = 0; nand_manuf_ids[maf_idx].id != 0x0; maf_idx++) {
		if (nand_manuf_ids[maf_idx].id == *maf_id)
			break;
	}

	/*
	 * Check, if buswidth is correct. Hardware drivers should set
	 * chip correct !
	 */
	if (busw != (chip->options & NAND_BUSWIDTH_16)) {
		PRINT("NAND device: Manufacturer ID:"
		       " 0x%02x, Chip ID: 0x%02x (%s)\n", *maf_id,
		       *dev_id, nand_manuf_ids[maf_idx].name);
		PRINT("NAND bus width %d instead %d bit\n",
		       (chip->options & NAND_BUSWIDTH_16) ? 16 : 8,
		       busw ? 16 : 8);
		return NULL;
	}

	/* Calculate the address shift from the page size */
	chip->page_shift = ffs(chip->info->pagesize) - 1;
	/* Convert chipsize to number of pages per chip -1. */
	chip->pagemask = (chip->info->chipsize >> chip->page_shift) - 1;

	chip->bbt_erase_shift = chip->phys_erase_shift =
		ffs(chip->info->pagesize) - 1;
	if (chip->info->chipsize & 0xffffffff)
		chip->chip_shift = ffs((unsigned)chip->info->chipsize) - 1;
	else {
		chip->chip_shift = ffs((unsigned)(chip->info->chipsize >> 32));
		chip->chip_shift += 32 - 1;
	}

	/* Set the bad block position */
	if (chip->info->pagesize> 512 || (busw & NAND_BUSWIDTH_16))
		chip->badblockpos = NAND_LARGE_BADBLOCK_POS;
	else
		chip->badblockpos = NAND_SMALL_BADBLOCK_POS;

	/*
	 * Bad block marker is stored in the last page of each block
	 * on Samsung and Hynix MLC devices; stored in first two pages
	 * of each block on Micron devices with 2KiB pages and on
	 * SLC Samsung, Hynix, Toshiba and AMD/Spansion. All others scan
	 * only the first page.
	 */
	if ((chip->cellinfo & NAND_CI_CELLTYPE_MSK) &&
			(*maf_id == NAND_MFR_SAMSUNG ||
			 *maf_id == NAND_MFR_HYNIX))
		chip->options |= NAND_BBT_SCANLASTPAGE;
	else if ((!(chip->cellinfo & NAND_CI_CELLTYPE_MSK) &&
				(*maf_id == NAND_MFR_SAMSUNG ||
				 *maf_id == NAND_MFR_HYNIX ||
				 *maf_id == NAND_MFR_TOSHIBA ||
				 *maf_id == NAND_MFR_AMD)) ||
			(chip->info->pagesize== 2048 &&
			 *maf_id == NAND_MFR_MICRON))
		chip->options |= NAND_BBT_SCAN2NDPAGE;

	/*
	 * Numonyx/ST 2K pages, x8 bus use BOTH byte 1 and 6
	 */
	if (!(busw & NAND_BUSWIDTH_16) &&
			*maf_id == NAND_MFR_STMICRO &&
			chip->info->pagesize== 2048) {
		chip->options |= NAND_BBT_SCANBYTE1AND6;
		chip->badblockpos = 0;
	}

	PRINT("NAND device: Manufacturer ID:"
		" 0x%02x, Chip ID: 0x%02x (%s)\n", *maf_id, *dev_id,
		nand_manuf_ids[maf_idx].name);


	chip->block_shift = ffs(chip->info->blocksize) - 1;

	chip->pages_per_block = chip->info->blocksize / chip->info->pagesize;

	return type;
}

/**
 * nand_scan_ident - [NAND Interface] Scan for the NAND device
 * @maxchips:	     Number of chips to scan for
 * @table:	     Alternative NAND ID table
  */
int nand_scan_ident(int maxchips,
		    struct nand_flash_dev *table)
{
	int i, busw, nand_maf_id, nand_dev_id;
	struct nand_flash_dev *type;
	uint8_t	IDs[8];

	/* Get buswidth to select the correct functions */
	busw = chip->options & NAND_BUSWIDTH_16;

	/* Read the flash type */
	type = nand_get_flash_type(busw,
				&nand_maf_id, &nand_dev_id, table);

	if (NULL == type) {
		PRINT("No NAND device found.\n");
		chip_select(-1);
		return -1;
	}

	/* Check for a chip array */
	for (i = 1; i < maxchips; i++) {
		chip_select(i);
		/* See comment in nand_get_flash_type for reset */
		nand_reset();
		/* Send the command for reading device ID */
		read_id(IDs, 8);
		/* Read manufacturer and device IDs */
		if (nand_maf_id != IDs[0] ||
		    nand_dev_id != IDs[1])
			break;
	}

	chip_select(-1);
	
	if (i > 1)
		PRINT("%d NAND chips detected\n", i);

	/* Store the number of chips and calc total size for mtd */
	chip->info->chips = i;
	chip->info->buswidth16 = (busw == 0) ? 0 : 1; 
	return 0;
}

static void enable_nand_clk()
{
	unsigned int val;

	val = *REG32(GBL_CFG_BUS_CLK_REG);
	if(((val & (0x1 << 16)) ^ (0x1 << 16)) != 0){
		val |= (0x1 << 16);
		*REG32(GBL_CFG_BUS_CLK_REG) = val;
	}

	val = *REG32(GBL_CFG_SOFTRST_REG);
	if(((val & ((0x1 << 10))) ^ ((0x1 << 10))) != 0){
		val |= (0x1 << 10);
		*REG32(GBL_CFG_SOFTRST_REG) = val;
	}
}

int platform_init_nand()
{
	static struct nand_priv nand_priv_info;
	struct nandflash_info* flash_info;
	static struct nandflash_info nf_info;

	chip = &nand_priv_info;
	memset(chip, 0, sizeof(*chip));
	chip->info = &nf_info;
	memset(&nf_info, 0, sizeof(struct nandflash_info));

	chip->badblockbits = 8;

	enable_nand_clk();
	
	flash_info = detect_nand_flash();
	if (flash_info == NULL) {
		PRINT("Unknown flash type!\n");
		return -1;
	}
	else
	{
		PRINT("nand detect success chip->info%x flash_info%x\n",chip->info,flash_info );
		PRINT("pagesize%d blocksize0x%x chipsize%dM buswidth%d chips%d oobsize%d page_shift%d pagemask0x%x\n", flash_info->pagesize,
		flash_info->blocksize, flash_info->chipsize, flash_info->buswidth16,
		flash_info->chips, flash_info->oobsize, flash_info->page_shift, flash_info->pagemask);
	}

	return 0;
}

