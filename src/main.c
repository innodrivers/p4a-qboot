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

extern unsigned long __linux_cmdline;
extern unsigned long __linux_machid;
extern unsigned long __linux_atag_addr;
extern unsigned long __linux_kernel_addr;

static char default_machid[] = "3300";
static char default_atag_addr[] = "0x46000100";
static char default_kernel_addr[] = "0x46008000";
static char default_cmdline[] = "console=ttyS1,115200 init=/linuxrc mem=32M cpu1_mem=64M@0x42000000 mbox_mem=2M@0x41800000";


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
	char *pmachid;
	char* pcmdline;
	char* patagaddr;
	char* pkerneladdr;

	pmachid = (char*)&__linux_machid;
	if (*pmachid == 0) {
		pmachid = default_machid;
	}

	pcmdline = (char*)&__linux_cmdline;
	if (*pcmdline == 0) {
		pcmdline = default_cmdline;
	}

	patagaddr = (char*)&__linux_atag_addr;
	if (*patagaddr == 0) {
		patagaddr = default_atag_addr;
	}

	pkerneladdr = (char*)&__linux_kernel_addr;
	if (*pkerneladdr == 0) {
		pkerneladdr = default_kernel_addr;
	}

	argv[i++] ="linux";
	argv[i++] = "-m";
	argv[i++] = pmachid;
	argv[i++] = "-a";
	argv[i++] = patagaddr;
	argv[i++] = "-c";
	argv[i++] = pcmdline;
	argv[i++] = "-k";
	argv[i++] = pkerneladdr;
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

	/* when running here, if the serial port press the
	 * character 'c', then enter the interactive mode,
	 * otherwise boot linux kernel directly.
	 */
	if (serial_getc() == 'c') {
		start_console();

	} else {
		boot_linux();
	}
	
	dead_loop();
	
	return 0;
}
