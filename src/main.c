/**
 * @file
 * @brief	bootloader main routine
 * @author	jimmy.li<lizhengming@innofidei.com>
 * @date	2010/07/01
 * 
 * Copyright (c) 2010 Innofidei Inc. All rights reserved.
 */
#include <types.h>
#include <common.h>
#include <hardware.h>
#include <iomux.h>
#include <uart.h>
#include <io.h>
#include <cmd.h>

extern void start_console(void);
extern int p4a_board_init(void);


static int p4a_init(void)
{
	*REG32(PMU_CTRL_REG) |= (1<<28);    //enable Watchdog reset whole chip

	p4a_board_init();

	return 0;
}

static void dead_loop(void)
{
	while(1)
		;
}

static void boot_linux(void)
{
	cmd_tbl_t *cmdtp;
	char *argv[16];
	int i = 0;

	argv[i++] ="linux";
	argv[i++] = "-m";
	argv[i++] = "3300";		//board id
	argv[i++] = "-a";
	argv[i++] = "0x46000100";	// atag address
	argv[i++] = "-c";
	argv[i++] = "console=ttyS1,115200 init=/linuxrc mem=32M cpu1_mem=64M@0x42000000 mbox_mem=2M@0x41800000";	//command line string
	argv[i++] = "-k";
	argv[i++] = "0x46008000";	//kernel address
	argv[i++] = "-b";

	cmdtp = find_cmd("linux");
	if (cmdtp != NULL){
		cmdtp->cmd(i, argv);
	}else{
		PRINT(" Command %s - Invalid Command!\n Please use \"help\" to get command list.\n", argv[0]);
	}

}

int main(void)
{
	p4a_init();

	PRINT("welcome to bootloader!\n");

#if 0
	start_console();
#else
	boot_linux();
#endif
	
	
	dead_loop();
	
	return 0;
}
