/**
 * @file
 * @brief	p4a FPGA board
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

static p4a_mux_pin_t muxpins_cfg[] = {
};

void p4a_soft_reset()
{
#define P4A_SOFT_RST_PERI_BIT (25)
	unsigned int  temp;
    	temp = GET_REG(GBL_CFG_SOFTRST_REG);
    	temp |= (0x1<<P4A_SOFT_RST_PERI_BIT);
    	SET_REG(GBL_CFG_SOFTRST_REG, temp );
}

int p4a_board_init(void)
{
	p4a_iomux_config(muxpins_cfg, ARRAY_SIZE(muxpins_cfg));
//	timer_init();

	SET_REG(P4A_AIC_PHYS+4, 0xFFFFFFFF); 	//mask all interrput.
	p4a_soft_reset();

	serial_init(1);

	return 0;
}

