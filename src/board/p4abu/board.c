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

	/* Nand Controller */
	MP_NFCEN0_PAD_NF_CEN0,
	MP_NFRBN0_PAD_NF_RBN0,
	MP_NFCLE_PAD_NF_CLE,
	MP_NFALE_PAD_NF_ALE,
	MP_NFWRN_PAD_NF_WRN,
	MP_NFRDN_PAD_NF_RDN,
	MP_NFD0_PAD_NF_D0,
	MP_NFD1_PAD_NF_D1,
	MP_NFD2_PAD_NF_D2,
	MP_NFD3_PAD_NF_D3,
	MP_NFD4_PAD_NF_D4,
	MP_NFD5_PAD_NF_D5,
	MP_NFD6_PAD_NF_D6,
	MP_NFD7_PAD_NF_D7,

	
};

int p4a_board_init(void)
{
	SET_REG(P4A_AIC_PHYS+4, 0xFFFFFFFF); 	//mask all interrput.
	p4a_iomux_config(muxpins_cfg, ARRAY_SIZE(muxpins_cfg));

	//UART2 as serial debug port
	serial_init(1);
	timer_init();

	//UART4W as serial debug port
	//serial_init(2);

	//NAND controller
	platform_init_nand();

	return 0;
}

