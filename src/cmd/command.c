/**
 * @file
 * @brief	command
 *     
 * Copyright (c) 2010 Innofidei Inc. All rights reserved.
 *
 */
#include <common.h>
#include <types.h>
#include <string.h>
#include <cmd.h>

extern unsigned long __cmdcall_start  __attribute__ ((weak));
extern unsigned long __cmdcall_end  __attribute__ ((weak));

cmd_tbl_t* find_cmd (const char* cmd)
{
	cmd_tbl_t *cmdtp;
	
	for (cmdtp = (cmd_tbl_t *)&__cmdcall_start;
		cmdtp != (cmd_tbl_t *)&__cmdcall_end;
		cmdtp++) {
		if (strcmp(cmd, cmdtp->name) == 0) {
			return cmdtp;
		}
	}

	return (cmd_tbl_t*)0;
}

/* build-in command */
static int cmd_show_help(int argc, char** argv)
{
	cmd_tbl_t *cmdtp;
	
	PRINT("All support commands:\n");
	for (cmdtp = (cmd_tbl_t *)&__cmdcall_start;
		cmdtp != (cmd_tbl_t *)&__cmdcall_end;
		cmdtp++) {
		PRINT("%s\t\t%s\n", cmdtp->name, cmdtp->help);
	}
	return 0;
}

INSTALL_CMD(help, cmd_show_help, "show help for all support command");
