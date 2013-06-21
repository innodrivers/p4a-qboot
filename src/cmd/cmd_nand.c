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


#define islower(x) (((x) >= 'a') && ((x) <= 'z'))
#define isupper(x) (((x) >= 'A') && ((x) <= 'Z'))
#define isalpha(x) (islower(x) || isupper(x))
#define toupper(c)	(islower(c) ? ((c) - 'a' + 'A') : (c))


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

	*num = memparse(p, &endptr);

	return *p != '\0' && *endptr == '\0';
}
static int arg_off(const char *arg, loff_t *off)
{
	if (str2off(arg, off))
		return 0;
	else
		return -1;
	
}

static unsigned char nand_cmd_help[] = 
	"usage: \n\
	nand readid\n\
	nand getinfo\n\
	nand setinfo <pagesize> <blocksize> <chipsize> <buswidth16> [chips]\n\
	nand erase <offset> <size>\n\
	nand write <buf> <offset> <size> [withoob]\n\
	nand read  <buf> <offset> <size> [withoob]\n\
\n\
	options:\n\
		buf: the address contained the data to write or to store the data read back\n\ 
		offset: address of the nand flash to write to or read from\n\
		size: data len to write to or read from nand\n\
		withoob: 0 not read or write with oob\n\
				 1 read or write with oob\n\
				 default 0\n\n\
	";

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
	
	if (argc == 5){
		withoob = (int)simple_strtoul(argv[4], NULL, 0);
	}
	 

	PRINT("nand write buffer%x offset%llx size%u [withoob%d]\n", buffer, offset, size, withoob);

	pi.from = offset;
	pi.buf = buffer;
	pi.len = size;
	pi.withoob = withoob;
	pi.callback = NULL;

	ret = nand_flash_write(&pi);
	
	if(ret){
		PRINT("nand write failed\n");
	}

	return ret;
}


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
	
	buffer = (uint8_t*)simple_strtoul(argv[1], NULL, 0);

	if (arg_off(argv[2], &offset) != 0) {
		PRINT("invalid params nand_read offset%s\n", argv[2]);
		return -1;
	}

	size = memparse(argv[3], NULL);
	
	if (argc == 5){
		withoob = (int)simple_strtoul(argv[4], NULL, 0);
	}
	
	ri.from = offset;
	ri.buf = buffer;
	ri.len = size;
	ri.withoob = withoob;
	ri.callback = NULL;

	ret = nand_flash_read(&ri);
	
	if (ret){
		PRINT("nand read failed\n");
	}

	return ret;

}

static int cmd_nand_readid(int argc, char** argv)
{
	uint8_t ID[6];
	char info[32];

	PRINT("nand readid!\n");

	nand_flash_readid(ID, 6);

	PRINT("%02x %02x %02x %02x %02x %02x\n", ID[0], ID[1], ID[2], ID[3], ID[4], ID[5]);
	return 0;
}


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
	
	nand_flash_set_flashinfo(&info);
	flash_info = nand_flash_get_flashinfo();
	
	return 0;
}

static int cmd_nand_getinfo(int argc, char** argv)
{
	char info[64];

	if(NULL == flash_info){
		flash_info = nand_flash_get_flashinfo();

		if(NULL == flash_info){
			PRINT("Unknown Nand Flash\n");
			return -1;
		}
	} 

	PRINT("%d 0x%x %dM %d %d\n", flash_info->pagesize,
		flash_info->blocksize, flash_info->chipsize, flash_info->buswidth16,
		flash_info->chips);
	
	return 0;
}


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
	
	size = memparse(argv[2], NULL);
	
	if (!flash_info) {
		flash_info = nand_flash_get_flashinfo();

		if(flash_info == NULL){
			PRINT("Unknown Nand Flash\n");
			return -1;
		}
	}

	if (offset != _ALIGN_DOWN(offset, (unsigned long long)flash_info->blocksize) ||
		size != _ALIGN_UP(size, (unsigned long long)flash_info->blocksize)) {
		PRINT("Invalid Arguments\n");
		return -1;
	}

	ei.from = offset;
	ei.len = size;
	ei.callback = NULL;

	ret = nand_flash_erase(&ei);
	if (ret < 0){
		PRINT("cmd_nand_erase failed\n");
	}
	else {
		char info[16];
		PRINT( "erase %d blocks, %d failed\n", ei.total, ei.fail);
	}

	PRINT("erase done!\n");
	
	return ret;
}

unsigned long long test_buf[1024 + 32];
static int cmd_test_nand_write(int argc, char** argv)
{
	int i;
	struct erase_info ei;
	struct program_info pi;
	int ret = 0;
	struct read_info ri;
	unsigned char * ps = (unsigned char *)test_buf;

	memset(test_buf, 0, sizeof(test_buf));

	ei.from = 52 * 1024 * 1024;/* erase offset 52M*/
	ei.len = 8 * 1024 * 1024;/* erase size 8 M*/
	ei.callback = NULL;
	PRINT("cmd_test_nand_write begin erase\n");
	ret = nand_flash_erase(&ei);
	
	if (ret < 0){
		PRINT("cmd_nand_erase failed\n");
		return -1;
	}
	else {
		char info[16];
		PRINT( "erase %d blocks, %d failed\n", ei.total, ei.fail);
	}
	
	PRINT("cmd_test_nand_write erase done\n");
	
	for (i = 0 ; i < 4096; i++){
	    ps[i] = (i & 0xff);
	}
	
	pi.from = 52 * 1024 * 1024;
	pi.buf = ps;
	pi.len = i;
	pi.withoob = 0;
	pi.callback = NULL;

	PRINT("cmd_test_nand_write	begin write \n");

	ret = nand_flash_write(&pi);
	
	if (ret){
		PRINT("nand write failed\n");
		return -1;
	}
	PRINT("cmd_test_nand_write	 write done \n");

	ri.from = 52 * 1024 * 1024;
	ri.buf = ps + 4096;
	ri.len = i;
	ri.withoob = 0;
	ri.callback = NULL;
	PRINT("cmd_test_nand_write	 read begin \n");

	ret = nand_flash_read(&ri);
	
	if (ret){
		PRINT("nand read failed\n");
		return -1;
	}

	for (i = 0; i < 4096; i++){
		if (ps[i] != ps[i + 4096]){
			PRINT("cmd_test_nand_write	 read back compare failed ps[%d] %d %d\n", i, ps[i],ps[i+4096]);
			PRINT("ps 0x%x %d\n", ps, ps[i + 4096 + 1]);
			ret = -1;
			break;
		}
		
	}
	PRINT("ps 0x%x \n", ps);

	if (0 == ret){
		PRINT("cmd_test_nand_write success\n");
	}
	else {
		PRINT("cmd_test_nand_write failed\n");
	}
	
}
INSTALL_CMD(test_nand_write, cmd_test_nand_write, "test nand write");

static int cmd_nand(int argc, char** argv)
{
	char* sub_cmd;
	uint8_t* buffer = NULL;
	loff_t offset = 0;
	size_t size = 0;
	int withoob = 0;
	int ret;
	int len = 0;
	int i;
	
	if (argc < 2){
		PRINT("nand cmd err lack of subcmd: \n%s\n", nand_cmd_help);
		return -1;
	}
	sub_cmd = argv[1];
	argc--;
	argv++;

	for (i = 0; i < strlen(sub_cmd); i++){
		if (isalpha(sub_cmd[i])){
			sub_cmd[i] = toupper(sub_cmd[i]);
		}
		else {
			PRINT("nand subcmd err: %s", sub_cmd);
			return -1;
		}
	}

	if (!strncmp(sub_cmd, "WRITE", 5)) {
		ret = cmd_nand_write(argc, argv);
	}
	else if (!strncmp(sub_cmd, "READID", 6)){
		ret = cmd_nand_readid(argc, argv);
	}
	else if (!strncmp(sub_cmd, "READ", 4)){
		ret = cmd_nand_read(argc, argv);
	}
	else if (!strncmp(sub_cmd, "ERASE", 5)){
		ret = cmd_nand_erase(argc, argv);
	}
	else if (!strncmp(sub_cmd, "SETINFO", 7)){
		ret = cmd_nand_setinfo(argc, argv);
	}
	else if (!strncmp(sub_cmd, "GETINFO", 7)){
		ret = cmd_nand_getinfo(argc, argv);
	}
	else {
		PRINT("unknown nand subcmd %s\n    %s", sub_cmd, nand_cmd_help);
		return -1;
	}
	
	return ret;
}
INSTALL_CMD(nand, 	cmd_nand, nand_cmd_help);

static int cmd_test_nand_oob(int argc, char** argv)
{
	int i;
	struct erase_info ei;
	struct program_info pi;
	int ret = 0;
	struct read_info ri;
	unsigned char * ps = (unsigned int)test_buf;
	unsigned int err_cnt = 0;

	memset(test_buf, 0, sizeof(test_buf));

	PRINT("cmd_test_nand_write_oob case1 begin erase\n");

	ei.from = 52 * 1024 * 1024;/* erase offset 52M*/
	ei.len = 8 * 1024 * 1024;/* erase size 8 M*/
	ei.callback = NULL;
	PRINT("begin erase\n");
	ret = nand_flash_erase(&ei);
	
	if (ret < 0){
		PRINT("erase failed\n");
		err_cnt++;
		goto case2;
	}
	else {
		char info[16];
		PRINT( "erase %d blocks, %d failed\n", ei.total, ei.fail);
	}
	
	PRINT(" erase done\n");
	
	for (i = 0 ; i < 4096 + 64 * 2; i++){
	    ps[i] = (i & 0xff);
	}
	
	pi.from = 52 * 1024 * 1024;
	pi.buf = ps;
	pi.len = i;
	pi.withoob = 1;
	pi.callback = NULL;

	PRINT("cmd_test_nand_write_oob	begin write \n");

	ret = nand_flash_write(&pi);
	
	if (ret){
		PRINT("nand write failed\n");
		err_cnt++;
		goto case2;
	}
	PRINT("cmd_test_nand_write_oob	 write done \n");

	ri.from = 52 * 1024 * 1024;
	ri.buf = ps + i;
	ri.len = i;
	ri.withoob = 1;
	ri.callback = NULL;
	PRINT("cmd_test_nand_write_oob	 read begin ri.buf 0x%x\n", ri.buf);

	ret = nand_flash_read(&ri);
	
	if (ret){
		PRINT("nand read failed\n");
		err_cnt++;
		goto case2;
	}

	for (i = 0; i < 4096 + 64 * 2; i++){
		if (ps[i] != ps[i + 4096 + 64 * 2]){
			PRINT("cmd_test_nand_write_oob	 read back compare failed ps[%d] %d %d\n", i, ps[i],ps[i + 4096 + 64 * 2]);
			PRINT("ps 0x%x %d\n", ps, ps[i + 4096 + 64 * 2 + 1]);
			ret = -1;
			err_cnt++;
			goto case2;
		}
	}

	PRINT("cmd_test_nand_write_oob case1 success\n");

case2:


	if (0 == ret){
		PRINT("cmd_test_nand_write_oob success\n");
	}
	else {
		PRINT("cmd_test_nand_write_oob failed\n");
	}
}
INSTALL_CMD(test_nand_write_oob, cmd_test_nand_oob, "test nand write oob");



