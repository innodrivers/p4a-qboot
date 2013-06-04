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


//#define wait_cmdQ_done()		wait_condition(NSR_CMDQ_DONE, WAITMODE_AND, 4000)
//#define wait_device_ready()		wait_condition(NSR_DEV_RDY, WAITMODE_AND, 4000)
#define wait_cmdQ_done()		wait_condition(NSR_CMDQ_DONE, WAITMODE_AND, 400000*4)
#define wait_device_ready()		wait_condition(NSR_DEV_RDY, WAITMODE_AND, 400000*4)

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

#if 0
static inline void wait_event_set(unsigned offset, uint32_t mask, time_t delay)
{
	polling_bits_set_any(rd_regl, offset, mask, delay);
}

static inline void wait_event_set_all(unsigned offset, uint32_t mask, time_t delay)
{
	polling_bits_set_all(rd_regl, offset, mask, delay);
}

static inline void wait_event_clear(unsigned offset, uint32_t mask, time_t delay)
{
	polling_bits_clear_any(rd_regl, offset, mask, delay);
}
#endif
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
	return (chip->info->pagesize > 1024);
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

static void chip_select(int chip)
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
	ASSERT_CS;

	nfc_clearfifo();
	
	//BUG_ON(idx > ARRAY_SIZE(cmds));
	ret = nfc_send_cmdQ(&cmds[0], idx);
	

	if(ret){
		PRINT("read ID failed\n");
		DEASSERT_CS;
		return ret;
	}

	for(i = 0; i < MIN(len, 8); i++)
	{
		buff[i] = nand_read_byte();
	}
	DEASSERT_CS;

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

	//BUG_ON(idx > ARRAY_SIZE(cmds));
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
	
	//BUG_ON(idx > ARRAY_SIZE(cmds));
	ret = nfc_send_cmdQ(&cmds[0], idx);

	return ret;
}

static int nfc_erase_block(int page_addr)
{
	//int isLargePage = (host->mtd.writesize > 512) ? 1 : 0;
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
	
	//BUG_ON(idx > ARRAY_SIZE(cmds));
	ret = nfc_send_cmdQ(&cmds[0], idx);
	
	return 0;
}

static int nfc_page_program(int column, int page_addr)
{
	//int isLargePage = (host->mtd.writesize > 512) ? 1 : 0;
	int isLargePage = is_large_page();
	struct nfc_cmd_entry cmds[15];
	int idx = 0;
	int ret;

	//BUG_ON(column!=0 && column!=host->mtd.writesize);
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

	//BUG_ON(idx > ARRAY_SIZE(cmds));
	ret = nfc_send_cmdQ(&cmds[0], idx);

	PRINT("NSR = 0x%08x\n", rd_regl(NFC_STATUS));

	return ret;
}

static int nfc_page_read(int column, int page_addr)
{
	//int isLargePage = (host->mtd.writesize > 512) ? 1 : 0;
	int isLargePage = is_large_page();
	struct nfc_cmd_entry cmds[15];
	int idx;
	int ret;

	chip->rOffset = column;
	
	//BUG_ON(column!=0 && column!=host->mtd.writesize);
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
		
	//BUG_ON(idx > ARRAY_SIZE(cmds));
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
//	struct nand_chip *chip = mtd->priv;
//	struct p4a_nand_host *host = chip->priv;
	
	memcpy(buf, (void*)(NAND_BUFFER_PHY_ADDRESS + chip->rOffset), len);
	chip->rOffset += len;
}

static int nand_verify_buf(const uint8_t *buf, int len)
{
//	struct nand_chip *chip = mtd->priv;
//	struct p4a_nand_host *host = chip->priv;
	int i, offset=chip->rOffset;

	for (i=0; i<len; i++)
		if (buf[i] != ((uint8_t*)NAND_BUFFER_PHY_ADDRESS)[offset+i])
			return -1;

	chip->rOffset += len;
	return 0;
}

static int p4a_nand_dev_ready()
{
	return 1;
}

/*static void p4a_nand_command(struct mtd_info* mtd, unsigned command, int column, int page_addr)
{
	struct nand_chip *chip = mtd->priv;
	struct p4a_nand_host *host = chip->priv;

	dev_dbg(host->dev, "command: 0x%x at address: 0x%x, column:0x%x\n", 
		command, page_addr, column);

	switch (command) {
	case NAND_CMD_READOOB:
		column += mtd->writesize;
	case NAND_CMD_READ0:	
		p4a_nfc_page_read(host, column, page_addr);
		break;
	case NAND_CMD_SEQIN:
		host->last_cmd = command;
		host->column = column;
		host->page_addr = page_addr;

		memset(host->dma_virt, 0xFF, MAX_PAGECACHE_SIZE);	
		host->wOffset = column;
		break;
	case NAND_CMD_PAGEPROG:
		p4a_nfc_page_program(host, host->column, host->page_addr);
		break;
	case NAND_CMD_ERASE1:
		p4a_nfc_erase_block(host, page_addr);
		break;
	case NAND_CMD_ERASE2:
		break;
	case NAND_CMD_READID:
		p4a_nfc_read_id(host);
		break;
	case NAND_CMD_STATUS:
		p4a_nand_read_status(host);
		break;
	case NAND_CMD_RESET:
		p4a_nand_reset(host);
		break;
	default:
		dev_warn(host->dev,"unknown command %d.\n", command);
		break;
	}
}*/

// wait for command done, applies to erase and program
// return nand status register value
static int nand_waitfunc()
{
//	struct nand_chip *chip = mtd->priv;
//	struct p4a_nand_host *host = chip->priv;
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
//	int i, addr_i = 0;
//	uint32_t avail = 0;
//	uint8_t *ptr = buff;
	size_t darea_len = 0;
	size_t sarea_len = 0;
//	uint32_t wait_event_mask;
	int status = 0;

	if (!info)
		return -1;

	darea_len = MIN(len, info->pagesize);
	if (extra_per_page) {
		sarea_len = MIN(chip->freeoob_bytes, len-darea_len);
	}

	PRINT("write_page: darea_len = %d, sarea_len = %d, hwecc=%d\n", darea_len, sarea_len, chip->hwecc);

	//if (info->buswidth16)
	//	column >>= 1;


	/* write data area data */
	if (darea_len > 0) {
		memset((void*)NAND_BUFFER_PHY_ADDRESS, 0xFF, MAX_PAGECACHE_SIZE);	
		
		nand_write_buf(buff, darea_len);
		nfc_page_program(column, page);

		status = nand_waitfunc();
		if (status & NAND_STATUS_FAIL)
		{
			PRINT("page program failed\n");
			return -1;
		}
	}

	/* write spare area data */
	if (sarea_len > 0) {
		column = info->pagesize;
		memset((void *)NAND_BUFFER_PHY_ADDRESS, 0xFF, MAX_PAGECACHE_SIZE);	
		
		nand_write_buf(buff + darea_len, sarea_len);
		nfc_page_program(column, page);
		
		status = nand_waitfunc();
		if (status & NAND_STATUS_FAIL)
		{
			PRINT("page program oob failed\n");
			return -1;
		}
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

	//if (info->buswidth16)
	//	column >>= 1;

	darea_len = MIN(len, info->pagesize);
	if (extra_per_page){
		sarea_len = MIN(chip->freeoob_bytes, len-darea_len);
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

	sarea_len = MIN(len, chip->freeoob_bytes);
	if (sarea_len < len) {
		memset(&buff[sarea_len], 0xff, len - sarea_len);
	}
	
	if (sarea_len > 0) {
		column = info->pagesize;
		nfc_page_read(column, page);
		nand_read_buf(buff, sarea_len);
	}
	
	return 0;

#if 0
	if (is_large_page()) {
		if (chip->hwecc)
			column = (info->pagesize / chip->sector_size) * (chip->sector_size + chip->hwecc_bytes);
		else
			column = info->pagesize;

		wr_regl(NFC_FACOMM_1, NAND_CMD_READ0);
		wr_regl(NFC_FACOMM_2, NAND_CMD_READSTART);
		wr_regl(NFC_FATCTL, FATCTL_AT_MODE4);	/* FACOMM_1 + Address + FACOMM_2 + Check Ready + Data */

	} else {
		if (chip->hwecc)
			column = chip->hwecc_bytes;
		else
			column = 0;
		wr_regl(NFC_FACOMM_1, NAND_CMD_READOOB);
		wr_regl(NFC_FATCTL, FATCTL_AT_MODE1);	/* FACOMM_1 + Address + Check Ready + Data */
	}

	if (info->buswidth16) {
		column >>= 1;
	}

	/* Address cycle */
	for (i=0; i < chip->column_cycles; i++) {
		wr_regl(NFC_FAx(addr_i++), column & 0xff);
		column >>= 8;
	}

	for (i=0; i < chip->row_cycles; i++) {
		wr_regl(NFC_FAx(addr_i++), page & 0xff);
		page >>= 8;
	}

	wr_regl(NFC_FCMDCTL, FCMDCTL_FACMD_MODE(addr_i - 1) | FCMDCTL_SET_FACMD);	/* trigger CMD/Address */
	wait_event_clear(NFC_FCMDCTL, FCMDCTL_SET_FACMD, 1000);

	ptr = buff;
	while (sarea_len) {
		int rlen = MIN(sarea_len, 46);
		wr_regl(NFC_FSPR_CNT, rlen -1);
		wr_regl(NFC_FDBACTL, FDBACTL_SET_FDBA | FDBACTL_FW_RDIR | FDBACTL_SPARE_ONLY | 0x4);
		wait_event_clear(NFC_FDBACTL, FDBACTL_SET_FDBA, 10000);
		for (i=0; i<rlen; i++)
			*ptr++ = (uint8_t)(rd_regl(NFC_FSPR_REGx(i)));

		sarea_len -= rlen;
	}


	return 0;
	#endif
}

/* if return 1, indicates a bad block */
static int check_bad_block(uint32_t page_addr)
{
	struct nandflash_info* info = chip->info;
	int column; 
	int i, addr_i = 0;
	uint8_t data;

#if 0 
	if (!info)
		return -1;
	
	/* ECC disable */
	wr_regl(NFC_ECCTL, 0);

	/* Command */
	if (is_large_page()) {
		column = (info->pagesize / chip->sector_size) * (chip->sector_size + chip->hwecc_bytes);
		wr_regl(NFC_FACOMM_1, NAND_CMD_READ0);
		wr_regl(NFC_FACOMM_2, NAND_CMD_READSTART);
		wr_regl(NFC_FATCTL, FATCTL_AT_MODE4);

	} else {
		column = chip->hwecc_bytes; 
		wr_regl(NFC_FACOMM_1, NAND_CMD_READOOB);
		wr_regl(NFC_FATCTL, FATCTL_AT_MODE1);
	}

	if (info->buswidth16)
		column >>= 1;

	/* Address cycle */
	for (i=0; i < chip->column_cycles; i++) {
		wr_regl(NFC_FAx(addr_i++), column & 0xff);
		column >>= 8;
	}

	for (i=0; i < chip->row_cycles; i++) {
		wr_regl(NFC_FAx(addr_i++), page_addr & 0xff);
		page_addr >>= 8;	
	}

	wr_regl(NFC_FSPR_CNT, 0);		/* read 1 spare data */
	wr_regl(NFC_FCMDCTL, FCMDCTL_FACMD_MODE(addr_i - 1));
	wr_regl(NFC_FDBACTL, FDBACTL_FW_RDIR | FDBACTL_SPARE_ONLY | 0x4);
	wr_regl(NFC_FATCTL, rd_regl(NFC_FATCTL) | FATCTL_AUTO_TRIGGER);		/* trigger CMD/Address/Data */

	wait_event_clear(NFC_FATCTL, FATCTL_AUTO_TRIGGER, 10000);

	data = rd_regl(NFC_FSPR_REGx(0));

	if (data != 0xff) {
		dprintf(INFO, "bad block at 0x%x\n", page_addr << chip->page_shift);
		return 1;
	}
	return 0;
	#endif 
	return 0;
}


/*----------------Nand Flash APIs----------------------*/
int nand_flash_readid(uint8_t *IDs, int num)
{
	return read_id(IDs, num);
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
	//ei->total = _ALIGN_UP(ei->len, (lsize_t)info->blocksize) / (lsize_t)info->blocksize;
	interval = MIN(ei->total, 10);

	block = lludiv(ei->from, info->blocksize);	/* start block address */
	//block = ei->from / info->blocksize;	/* start block address */

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
			if (check_bad_block(page)) {
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
			if (check_bad_block(page)) {
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
	return chip->info;
}

int nand_flash_set_flashinfo(struct nandflash_info* _info)
{
	struct nandflash_info *info = chip->info;
	int update = 0;

	if (!info) {
		static struct nandflash_info nf_info;
		info = &nf_info;
		//info = (struct nandflash_info*)malloc(sizeof(struct nandflash_info));
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

	flash_info = nand_flash_get_flashinfo();	
	if (flash_info == NULL) {
		uint8_t IDs[5];
		struct nandflash_info info;

		nand_flash_readid(IDs, 5);
		//TODO : parse NAND

		info.pagesize = 2048;
		info.blocksize = KiB(128);
		info.chipsize = 128;
		info.buswidth16 = 0;
		info.chips = 1;
		nand_flash_set_flashinfo(&info);

		flash_info = nand_flash_get_flashinfo();
	}

	return flash_info;

}

int platform_init_nand()
{
	static struct nand_priv nand_priv_info;
	struct nandflash_info* flash_info;
	unsigned int val;
	//clock_switch(CLK_NAND, 1);
	/* enable nand clock */
	/*val = *REG32(PMU_CLKRST1_REG);
	if(((val & ((0x1 << 5))) ^ ((0x1 << 5))) != 0){
		val |= (0x1 << 5);
		*REG32(GBL_CFG_BUS_CLK_REG) = val;
	}*/

	
	/*val = *REG32(PMU_CLKRST1_REG);
	if(((val & ((0x1 << 5))) ^ ((0x1 << 5))) != 0){
		val |= (0x1 << 5);
		*REG32(PMU_CLKRST1_REG) = val;
	}*/
	
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

	//chip = (struct nand_priv*)malloc(sizeof(struct nand_priv));
	chip = &nand_priv_info;

	memset(chip, 0, sizeof(*chip));

	flash_info = detect_nand_flash();
	if (flash_info == NULL) {
		PRINT("Nand Flash Unknown!\n");
		return;
	}

	return 0;
}

