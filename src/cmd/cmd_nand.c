/**
 * @file
 * @brief	jump to address
 *     
 * Copyright (c) 2010 Innofidei Inc. All rights reserved.
 *
 */
#include <common.h>
#include <types.h>
#include <string.h>
#include <cmd.h>
#include <hardware.h>
#include <io.h>
#include <clk.h>
#include <nand/nand.h>

static struct nandflash_info *flash_info;

/* align x on a size boundary - adjust x up/down if needed */
#define _ALIGN_UP(x, size)    (((x)+((size)-1)) & (~((size)-1)))
#define _ALIGN_DOWN(x, size)  ((x) & (~((size)-1)))


/*
 * parse a string with mem suffixes into a number
 *
 * @ptr : where parse begins
 * @retptr: Optional pointer to next char after parse completes
 */
unsigned long long memparse(const char* ptr, char **retptr)
{
	char* endptr;
	
	unsigned long long ret = simple_strtoul(ptr, &endptr, 0);

	switch (*endptr) {
	case 'G':
	case 'g':
		ret <<= 10;
	case 'M':
	case 'm':
		ret <<= 10;
	case 'K':
	case 'k':
		ret <<= 10;
		endptr++;
	default:
		break;
	}

	if (retptr)
		*retptr = endptr;
	
	return ret;
}


/* convert string to offset value
 * if return 1, means string to offset value valid 
 */
static int str2off(const char* p, loff_t *num)
{
	char* endptr;

//	*num = strtoull(p, &endptr, 0);
	*num = memparse(p, &endptr);

	return *p != '\0' && *endptr == '\0';
}
#if 0
/*
 * convert partition name to its offset value
 * input string format should be : "<partname>[:offset]"
 */
static int part2off(const char *instr, loff_t *num)
{
	struct nandflash_ptentry *p;
	char* split;
	char partname[MAX_PTENTRY_NAME];
	loff_t offset = 0;

	split = strchr(instr, ':');
	if (split == NULL) {
		strcpy(partname, instr);
	} else {
		int c = split - instr;
		strncpy(partname, instr, c);
		partname[c] = '\0';
		offset = memparse(split+1, NULL);
	}

	p = find_nand_part(get_nand_ptable(), partname);
	if (!p) {
		PRINT(INFO, "part %s not found!\n", partname);
		return -1;
	}

	*num = p->offset + offset;
	return 0; 
}
#endif
static int arg_off(const char *arg, loff_t *off)
{
	if (str2off(arg, off))
		return 0;
	else
		return -1;
	
	//return part2off(arg, off);
}





static int cmd_nand_write(int argc, char** argv)
{
	uint8_t* buffer = NULL;
	loff_t offset = 0;
	size_t size = 0;
	int withoob = 0;
	struct program_info pi;
	int ret;

	if (argc < 4 || argc > 5) {
		PRINT("invalid params nand_write buffer offset size [withoob]\n");
		return -1;
	}

	buffer = (uint8_t*)simple_strtoul(argv[1], NULL, 0);

	if (arg_off(argv[2], &offset) != 0) {
		PRINT("invalid params nand_write offset%s\n", argv[2]);
		return -1;
	}

	size = memparse(argv[3], NULL);
	
	if(argc == 5)
	{
		withoob = (int)simple_strtoul(argv[4], NULL, 0);
	}
	 

	PRINT("nand write buffer%x offset%llx size%u [withoob%d]\n", buffer, offset, size, withoob);

	pi.from = offset;
	pi.buf = buffer;
	pi.len = size;
	pi.withoob = withoob;
	//pi.callback = nand_write_callback;
	pi.callback = NULL;

	ret = nand_flash_write(&pi);
	
	if(ret){
		PRINT("nand write failed\n");
	}

	return ret;
}

INSTALL_CMD(nand_write, 	cmd_nand_write, "nand write");

static int cmd_nand_read(int argc, char** argv)
{

	uint8_t* buffer = NULL;
	loff_t offset = 0;
	size_t size = 0;
	int withoob = 0;
	struct read_info ri;
	int ret;

	if (argc < 4 || argc > 5) {
		PRINT("invalid params nand_read buffer offset size [withoob]\n");
		return -1;
	}

	//buffer = (uint8_t*)simple_strtoul(argv[1], NULL, 0);
	//offset = (unsigned int*)simple_strtoull(argv[2], NULL, 0);
	//size = (loff_t)simple_strtoul(argv[3], NULL, 0);

	
	buffer = (uint8_t*)simple_strtoul(argv[1], NULL, 0);

	if (arg_off(argv[2], &offset) != 0) {
		PRINT("invalid params nand_read offset%s\n", argv[2]);
		return -1;
	}

	size = memparse(argv[3], NULL);
	
	if(argc == 5)
	{
		withoob = (int)simple_strtoul(argv[4], NULL, 0);
	}
	
	if(argc == 5)
	{
		withoob = (int)simple_strtoul(argv[4], NULL, 0);
	}
	 

	PRINT("nand read buffer%x offset%llx size%u [withoob%d]\n", buffer, offset, size, withoob);

	ri.from = offset;
	ri.buf = buffer;
	ri.len = size;
	ri.withoob = withoob;
	//ri.callback = nand_read_callback;
	ri.callback = NULL;

	ret = nand_flash_read(&ri);
	
	if(ret){
		PRINT("nand read failed\n");
	}

	return ret;

}

INSTALL_CMD(nand_read, 	cmd_nand_read, "nand read");

static int cmd_nand_readid(int argc, char** argv)
{
	uint8_t ID[6];
	char info[32];

	PRINT("nand readid!\n");

	nand_flash_readid(ID, 6);

	//snprintf(info, 32, "%02x %02x %02x %02x %02x %02x", ID[0], ID[1], ID[2], ID[3], ID[4], ID[5]);
	PRINT("%02x %02x %02x %02x %02x %02x\n", ID[0], ID[1], ID[2], ID[3], ID[4], ID[5]);
	return 0;
}

INSTALL_CMD(nand_readid, 	cmd_nand_readid, "nand readid");

static int cmd_nand_setinfo(int argc, char** argv)
{
	struct nandflash_info info;
	if (argc < 4) {
		PRINT("invalid params nand_setinfo pagesize blocksize chipsize buswidth16 chips \n");
		return -1;
	}
	
	info.pagesize = memparse(argv[1], NULL);
	info.blocksize = memparse(argv[2], NULL);
	info.chipsize = memparse(argv[3], NULL) >> 20;
	info.buswidth16 = simple_strtoul(argv[4], NULL, 0);
	info.chips = 1;
	
	if (argc > 5) {
		info.chips = simple_strtoul(argv[5], NULL, 0);
	}
	
	
	PRINT("nand setinfo!\n");

	nand_flash_set_flashinfo(&info);

	flash_info = nand_flash_get_flashinfo();

	return 0;
}

INSTALL_CMD(nand_setinfo, 	cmd_nand_setinfo, "nand set info");

static int cmd_nand_getinfo(int argc, char** argv)
{
	if (flash_info == NULL) {
				PRINT("Unknown Nand Flash\n");
	
	} else {
		char info[64];

		//snprintf(info, 64, "%d 0x%x %dM %d %d", flash_info->pagesize,
		//	flash_info->blocksize, flash_info->chipsize, flash_info->buswidth16,
		//	flash_info->chips);
		//usbflasher_okay(info);

		
		PRINT("%d 0x%x %dM %d %d\n", flash_info->pagesize,
			flash_info->blocksize, flash_info->chipsize, flash_info->buswidth16,
			flash_info->chips);
	}
	return 0;
}

INSTALL_CMD(nand_getinfo, 	cmd_nand_getinfo, "nand get info");


static int cmd_nand_erase(int argc, char** argv)
{
	loff_t offset;
	lsize_t size;
	struct erase_info ei;
	int ret;

	if (argc != 3) {
		PRINT("invalid params nand_erase <offset> <size> \n");
		return -1;

	}
	
	if (arg_off(argv[1], &offset) != 0) {
		PRINT("invalid params nand_erase <offset> <size> \n");
		return -1;
	}
	
	//offset = memparse(argv[1], NULL);
	size = memparse(argv[2], NULL);
	
	PRINT("nand erase! hex %x %x %x\n", *((char*)argv[1]), *((char*)argv[1] + 1), *((char*)argv[1] + 2));
	PRINT("nand erase! char %c %c %c\n", *((char*)argv[1]), *((char*)argv[1] + 1), *((char*)argv[1] + 2));

	PRINT("nand erase! offset(%x), size(%x) argv[0]%s argv[1]%s argv[2]%s\n", (unsigned int)offset, (unsigned int)size, argv[0],argv[1],argv[2]);



//	PRINT("nand erase! offset(%llx), size(%llx) argv[0]%s argv[1]%s argv[2]%s\n", offset, size, argv[0],argv[1],argv[2]);

	if (!flash_info) {
		PRINT("Unknown Nand Flash\n");
		return -1;
	}

	if (offset != _ALIGN_DOWN(offset, (unsigned long long)flash_info->blocksize) ||
		size != _ALIGN_UP(size, (unsigned long long)flash_info->blocksize)) {
		PRINT("Invalid Arguments\n");
		return -1;
	}

	ei.from = offset;
	ei.len = size;

	//ei.callback = nand_erase_callback;
	ei.callback = NULL;

	ret = nand_flash_erase(&ei);
	if (ret < 0)
	{
		PRINT("cmd_nand_erase failed\n");
	}
	else {
		char info[16];
		//snprintf(info, 16, "erase %d blocks, %d failed", ei.total, ei.fail);
		PRINT( "erase %d blocks, %d failed\n", ei.total, ei.fail);
	}

	PRINT("erase done!\n");
	
	return ret;
}

INSTALL_CMD(nand_erase, cmd_nand_erase, "nand erase");


#define NAND_TEST_DATA_BUFF (0x4700000 + 8192 + 576)

unsigned long long test_buf[1024];
//INSTALL_CMD(test_nand_erase, cmd_test_nand_erase, "test nand erase");

static int cmd_test_nand_write(int argc, char** argv)
{
	unsigned char* pbuf = (unsigned char*)NAND_TEST_DATA_BUFF;
	int i;
	struct erase_info ei;
	struct program_info pi;
	int ret = 0;
	struct read_info ri;
	unsigned char * ps = (unsigned char *)test_buf;

	ei.from = 52 * 1024 * 1024;/* erase offset 52M*/
	ei.len = 8 * 1024 * 1024;/* erase size 8 M*/

	//ei.callback = nand_erase_callback;
	ei.callback = NULL;
	PRINT("cmd_test_nand_write begin erase\n");
	ret = nand_flash_erase(&ei);
	
	if (ret < 0)
	{
		PRINT("cmd_nand_erase failed\n");
		return -1;
	}
	else {
		char info[16];
		//snprintf(info, 16, "erase %d blocks, %d failed", ei.total, ei.fail);
		PRINT( "erase %d blocks, %d failed\n", ei.total, ei.fail);
	}
	PRINT("cmd_test_nand_write  erase done\n");
	
	
	for(i = 0 ; i < 4096; i++)
	{
	    ps[i] = (i & 0xff);
		//pbuf[i] = i;
		//()test_buf[]
	}
	
	pi.from = 52 * 1024 * 1024;
	pi.buf = ps;
	pi.len = i;
	pi.withoob = 0;
	//pi.callback = nand_write_callback;
	pi.callback = NULL;


	PRINT("cmd_test_nand_write	begin write \n");

	ret = nand_flash_write(&pi);
	
	if(ret){
		PRINT("nand write failed\n");
		return -1;
	}
	PRINT("cmd_test_nand_write	 write done \n");


	ri.from = 52 * 1024 * 1024;
	ri.buf = ps + 4096;
	ri.len = i;
	ri.withoob = 0;
	//ri.callback = nand_read_callback;
	ri.callback = NULL;
	PRINT("cmd_test_nand_write	 read begin \n");

	ret = nand_flash_read(&ri);
	
	if(ret){
		PRINT("nand read failed\n");
		return -1;
	}

	for(i = 0; i < 4096; i++){
		if(ps[i] != ps[i + 4096])
		{
			PRINT("cmd_test_nand_write	 read back compare failed ps[%d] %d %d\n", i, ps[i],ps[i+4096]);
			ret = -1;
			return ret;
		}
		
	}

	if(0 == ret)
	{
		PRINT("cmd_test_nand_write success");
	}
	else
	{
		PRINT("cmd_test_nand_write failed");
	}

	
	
}
INSTALL_CMD(test_nand_write, cmd_test_nand_write, "test nand write");

//INSTALL_CMD(test_nand_read, cmd_test_nand_read, "test nand read");

