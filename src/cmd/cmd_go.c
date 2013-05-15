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


typedef void (*jump_fn_t)(void);

static int cmd_go(int argc, char** argv)
{
	unsigned long address;
	jump_fn_t jump_to;

	if (argc != 2) {
		PRINT("go <address>\n");
		return -1;
	}

	address = simple_strtoul(argv[1], NULL, 0);

	PRINT("jump to %08x\n", address);
	jump_to = (jump_fn_t)address;

	jump_to();
	return 0;
}

INSTALL_CMD(go, 	cmd_go, "jump to specific address");

