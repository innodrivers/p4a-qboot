/**
 * @file
 * @brief	memory/register dump
 *     
 * Copyright (c) 2010 Innofidei Inc. All rights reserved.
 *
 */
#include <common.h>
#include <types.h>
#include <string.h>
#include <cmd.h>
#include <io.h>

static void dump_mem_byte(unsigned long addr, int size)
{
	int i;
	volatile unsigned char *p = (volatile unsigned char*)addr;
	
	for (i=0; i<size; i++) {
		if ((i % 16) == 0)
			PRINT("%08x :  ", &p[i]);
			
		PRINT("%02x ", p[i]);

		if ((i + 1) % 16 == 0)
			PRINT("\n");
	}
}

static void dump_mem_word(unsigned long addr, int size)
{
	int i;
	volatile unsigned short *p = (volatile unsigned short*)addr;
	
	for (i=0; i<(size>>1); i++) {
		if ((i % 8) == 0)
			PRINT("%08x :  ", &p[i]);

		PRINT("%04x ", p[i]);

		if ((i + 1) % 8 == 0)
			PRINT("\n");
	}
}

static void dump_mem_dword(unsigned long addr, int size)
{
	int i;
	volatile unsigned long *p = (volatile unsigned long*)addr;
	
	for (i=0; i<(size>>2); i++) {
		if ((i % 4) == 0)
			PRINT("%08x :  ", &p[i]);

		PRINT("%08x ", p[i]);

		if ((i + 1) % 4 == 0)
			PRINT("\n");
	}
}

static int cmd_memdump(int argc, char** argv)
{
	unsigned long addr;
	int size = 4;

	if (argc == 1) {
		PRINT("%s <address> [length]\n", argv[0]);
		return -1;
	}

	addr = simple_strtoul(argv[1], NULL, 0);
	if (argc >2)
		size = simple_strtoul(argv[2], NULL, 0);

	PRINT("Dump Memory from 0x%08x , size %d :\n", addr, size);

	if (argv[0][2] == 'l') {
		dump_mem_dword(addr, size);
	} else if (argv[0][2] == 'w') {
		dump_mem_word(addr, size);
	} else {
		dump_mem_byte(addr, size);
	}
	return 0;
}

static int cmd_memwrite(int argc, char** argv)
{
	unsigned long addr, val;

	if (argc < 3) {
		PRINT("%s <address> <value>\n", argv[0]);
	}

	addr = simple_strtoul(argv[1], NULL, 0);
	val = simple_strtoul(argv[2], NULL, 0);

	if (argv[0][2] == 'l') {
		*REG32(addr) = val;

	} else if (argv[0][2] == 'w') {
		*REG16(addr) = (unsigned short)val;
		
	} else {
		*REG8(addr) = (unsigned char)val;
	}

	return 0;
}

static int cmd_memtest(int argc, char** argv)
{
	volatile unsigned long *addr, *start, *end;
	unsigned long   val;
	unsigned long readback;
	int     rcode = 0;

	unsigned int   incr;
	unsigned int   pattern;


	if (argc < 3){
		PRINT("Usage:\n");
		PRINT("%s <mem_start> <mem_end> [pattern]\n", argv[0]);
		return 0;
	}
    
	start = (unsigned long *)simple_strtoul(argv[1], NULL, 0);
	end = (unsigned long *)simple_strtoul(argv[2], NULL, 0);
  
	if (argc > 3) {
  		pattern = (unsigned int)simple_strtoul(argv[3], NULL, 0);
	} else {
		pattern = 0;
	}

	PRINT("test memory from 0x%08x to 0x%08x, with init pattern 0x%08x\n", 
			start, end, pattern);

	incr = 1;
	for (;;) {
		PRINT ("Pattern %08X  Writing..."
			"%12s"
			"\b\b\b\b\b\b\b\b\b\b", pattern, "");

    for (addr=start,val=pattern; addr<end; addr++) {
			*addr = val;
			val  += incr;
		}

		PRINT("Reading...\n");

		for (addr=start,val=pattern; addr<end; addr++) {
			readback = *addr;
			if (readback != val) {
				PRINT ("Mem error @ 0x%08x: found %08x, expected %08x\n",
                    (unsigned int)addr, readback, val);
				rcode += 1;
				return rcode;
			}
			val += incr;

		}

		/*
		 * Flip the pattern each time to make lots of zeros and
		 * then, the next time, lots of ones.  We decrement
		 * the "negative" patterns and increment the "positive"
		 * patterns to preserve this feature.
		 */
		if(pattern & 0x80000000) {
			pattern = -pattern; /* complement & increment */
		} else {
			 pattern = ~pattern;
		}
		incr = -incr;

	}

	return rcode;
}

INSTALL_CMD(mdl, cmd_memdump, "memory dump in DWORD");
INSTALL_CMD(mdw, cmd_memdump, "memory dump in WORD");
INSTALL_CMD(mdb, cmd_memdump, "memory dump in BYTE");

INSTALL_CMD(mw, cmd_memwrite, "memory write in DWORD");
INSTALL_CMD(mww, cmd_memdump, "memory write in WORD");
INSTALL_CMD(mwb, cmd_memdump, "memory write in BYTE");

INSTALL_CMD(memtest, 	cmd_memtest, "test memory");

