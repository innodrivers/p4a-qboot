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

#define RESTART_MAGIC		0x76

#define WTLR		0x00		// Load
#define WTCRR		0x04		// Counter Restart
#define WTCR		0x08		// Control
#define WTRPR		0x0c		// Reset Pulse Length
#define WTCONR		0x10		// Counter
#define WTISR		0x14		// Interrupt Status

static uint32_t rd_regl(unsigned offset)
{
	return __raw_readl(WDT_BASE_ADDR + offset);
}

static void wr_regl(unsigned offset, uint32_t val)
{
	__raw_writel(val, WDT_BASE_ADDR + offset);
}

static int cmd_wdt(int argc, char** argv)
{
	unsigned long msec;
	uint32_t val;
	uint32_t loadval;
	unsigned long rate = get_APB_clk();
	unsigned hdiv, ldiv;
	unsigned long wdt_rate;

	if (argc != 2) {
		PRINT("wdt msec\n");
		return -1;
	}

	msec = simple_strtoul(argv[1], NULL, 0);

	PRINT("set watchdog timer %d msec\n", msec);

	clock_switch(CLK_WDT, 1);
	//wr_regl(WTCR, 0);

	wr_regl(WTCR, (13<<4));
	wdt_rate = rate / 16384;
	loadval = msec * wdt_rate / 1000; 
	PRINT("loadval = %d\n", loadval);
	
	wr_regl(WTLR, loadval);
	wr_regl(WTRPR, 10);
	
	val = rd_regl(WTCR);
	val |= (1<<9);
//	val |= (1<<8);
	wr_regl(WTCR, val);
	wr_regl(WTCRR, RESTART_MAGIC);


//	while (1)
//		PRINT("%08x\n", rd_regl(WTCONR));

	PRINT("R00h = %08x\n", rd_regl(0));
	PRINT("R04h = %08x\n", rd_regl(0x4));
	PRINT("R08h = %08x\n", rd_regl(0x8));
	PRINT("R0ch = %08x\n", rd_regl(0xc));
	PRINT("R10h = %08x\n", rd_regl(0x10));
	return 0;
}

INSTALL_CMD(wdt, 	cmd_wdt, "start watch dog timer");

