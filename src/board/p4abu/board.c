/**
 * @file
 * @brief	p4a bringup board
 * @author	jimmy.li<lizhengming@innofidei.com>
 * @date	2013/05/08
 * 
 * Copyright (c) 2013 Innofidei Inc. All rights reserved.
 */
#include <types.h>
#include <common.h>
#include <hardware.h>
#include <iomux.h>
#include <uart.h>
#include <io.h>
#include <cmd.h>

static p4a_mux_pin_t muxpins_cfg[] = {	/* p4a bringup */
	/* UART 2 */
	MP_UART2RX_PAD_UART2_RX,
	MP_UART2TX_PAD_UART2_TX,

	/* UART 3 (UART4W) */
	MP_UART3RX_PAD_UART3_RX,
	MP_UART3TX_PAD_UART3_TX,
};

int p4a_board_init(void)
{
	p4a_iomux_config(muxpins_cfg, ARRAY_SIZE(muxpins_cfg));

	//UART2 as serial debug port
	serial_init(1);

	//UART4W as serial debug port
	//serial_init(2);

	return 0;
}

