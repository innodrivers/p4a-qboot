/**
 * @file
 * @brief	delay times
 *     
 * Copyright (c) 2010 Innofidei Inc. All rights reserved.
 *
 */
#include <common.h>
#include <types.h>
#include <string.h>
#include <cmd.h>

static int show_help(void)
{
	PRINT("delay [-s seconds] [-m milliseconds] [-u microseconds]\n");
	PRINT("\t-s Seconds\n"
		"\t-m Milliseconds\n"
		"\t-u Microseconds\n");

	return 0;
}

static int cmd_delay(int argc, char** argv)
{
	int sec = 0;
	int msec = 0;
	int usec = 0;

	if (argc == 1)
		return show_help();

	if (strcmp(argv[1], "-s") == 0) {
		sec = simple_strtoul(argv[2], NULL, 0);
	} else if (strcmp(argv[1], "-m") == 0) {
		msec = simple_strtoul(argv[2], NULL, 0);
	} else if (strcmp(argv[1], "-u") == 0) {
		usec = simple_strtoul(argv[2], NULL, 0);
	}

	while (sec) {
		udelay(1000 * 1000);	
		sec--;
	}

	while (msec) {
		udelay(1000);
		msec--;
	}

	if (usec)
		udelay(usec);

	return 0;
}

INSTALL_CMD(delay, 	cmd_delay, "delay specific times");

