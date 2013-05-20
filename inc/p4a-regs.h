#ifndef _P4A_REGS_H
#define _P4A_REGS_H


/* Interrupt Controller */
#ifdef CONFIG_P4A_CPU2
#define P4A_AIC_PHYS				0xE0105000
#elif defined(CONFIG_P4A_CPU1)
#define P4A_AIC_PHYS				0xE0102000
#endif

#define ARM1                    0
#define ARM2                    1
#define M0                      2
#define CPU_ID		ARM2

/* Global */
#define P4A_GLOBAL_PHYS				0xE1200000

/* PMU */
#define P4A_PMU_PHYS				0xE1300000
#define P4A_PMU_BASE				(P4A_PMU_PHYS)

/* Peripheral Common */
#define P4A_PERI_PHYS				0xE0103000
#define PER_BASE_ADDRESS         		 (P4A_PERI_PHYS)

#define P4A_PERI_REG(off)			(P4A_PERI_PHYS + (off))

#define P4A_TIMER1_PHYS				P4A_PERI_REG(0x0)

#define P4A_TIMER2_PHYS				P4A_PERI_REG(0x10)

#define P4A_WDT_PHYS				P4A_PERI_REG(0x20)

#define P4A_UART1_PHYS				P4A_PERI_REG(0x40)	// 2-Wire UART

#define P4A_UART2_PHYS				P4A_PERI_REG(0x54)	// 2-Wire UART

#define P4A_I2C1_PHYS				P4A_PERI_REG(0xc0)

/* GPIO 1 ~ 6 */
#define P4A_GPIO1_PHYS				P4A_PERI_REG(0x100)

#define P4A_GPIO2_PHYS				P4A_PERI_REG(0x154)

#define P4A_GPIO3_PHYS				P4A_PERI_REG(0x180)

#define P4A_GPIO4_PHYS				P4A_PERI_REG(0x1c0)

#define P4A_GPIO5_PHYS				P4A_PERI_REG(0x2c0)

#define P4A_GPIO6_PHYS				P4A_PERI_REG(0x300)

/* 4-Wire UART*/
#define P4A_UART4W_PHYS				0xE0F00000

/* P4 Timer */
#ifdef CONFIG_P4A_CPU2
#define P4A_P4TIMER_PHYS			0xEF002000

#elif defined(CONFIG_P4A_CPU1)
#define P4A_P4TIMER_PHYS			0xEF001000
#endif

/* Mailbox */
#define P4A_MAILBOX_PHYS			0xE0700000

/* Ethernet */
#define P4A_ETHER_PHYS				0xE0300000

//////////////////////////////////////////////////////////////////////////////////////

/* Global Registers */
#define GBL_REG(off)		(P4A_GLOBAL_PHYS + (off))

#define GBL_CFG_ARM_CLK_REG		GBL_REG(0x0c)
#define GBL_CFG_DSP_CLK_REG		GBL_REG(0x10)
#define GBL_CFG_BUS_CLK_REG		GBL_REG(0x14)
#define GBL_CFG_AB_CLK_REG		GBL_REG(0x18)
#define GBL_CFG_PERI_CLK_REG	GBL_REG(0x1c)
#define GBL_CFG_SOFTRST_REG		GBL_REG(0x20)
#define GBL_BOOT_SEL_REG		GBL_REG(0x24)
#define GBL_CFG_DSP_REG			GBL_REG(0x28)
#define GBL_GPR_0_REG			GBL_REG(0x2c)
#define GBL_GPR_1_REG			GBL_REG(0x30)
#define GBL_GPR_2_REG			GBL_REG(0x34)
#define GBL_GPR_3_REG			GBL_REG(0x38)
#define GBL_CTRL_DDR_REG		GBL_REG(0x3c)
#define GBL_CTRL_DBG_REG		GBL_REG(0x40)
#define GBL_ARM_RST_REG			GBL_REG(0x6c)
#define GBL_CTRL_TOP_REG		GBL_REG(0x70)
#define GBL_CFG_ARM2_CLK_REG	GBL_REG(0x74)
#define GBL_CFG_PERI2_CLK_REG	GBL_REG(0x78)
#define GBL_CFG_DSP2_CLK_REG	GBL_REG(0x7c)
#define GBL_SMID_OCR_REG		GBL_REG(0x100)
#define GBL_SMID_CTL_REG		GBL_REG(0x104)
#define GBL_A8_MAP_ADDR_REG		GBL_REG(0x108)
#define GBL_PERI_HTRANS_REG		GBL_REG(0x160)

/* PMU Registers */
#define PMU_REG(off)		(P4A_PMU_BASE + (off))

#define PMU_CTRL_REG			PMU_REG(0x00)
#define PMU_PLL1_CTRL_REG		PMU_REG(0x74)
#define PMU_PLL2_CTRL_REG		PMU_REG(0x78)
#define PMU_PLL3_CTRL_REG		PMU_REG(0xa4)

#define PMU_CLKRST1_REG			PMU_REG(0x250)
#define PMU_CLKRST2_REG			PMU_REG(0x254)
#define PMU_CLKRST3_REG			PMU_REG(0x258)
#define PMU_CLKRST4_REG			PMU_REG(0x25c)
#define PMU_CLKRST5_REG			PMU_REG(0x260)



/*----------------LowLevel Debug UART -------------------------*/
#define UMR_OFF		0x00
#define URXR_OFF	0x04
#define UTXR_OFF	0x08
#define USR_OFF		0x0c
#define UCR_OFF		0x10

#ifdef CONFIG_P4A_LL_DEBUG_UART4W
#define LL_DBG_UART_PHYS		P4A_UART4W_PHYS
#define LL_DBG_UART_BASE		P4A_UART4W_BASE

#define LL_DBG_UMR_VALUE(_clock_, _baudrate_)      ( ((_clock_) << 4)/(_baudrate_) - 256 )
#define LL_DBG_USR_TFLEVEL_SHIFT	(7)
#define LL_DBG_USR_TFLEVEL_MASK 	(0x7f<<LL_DBG_USR_TFLEVEL_SHIFT)
#define LL_DBG_UTX_FIFOSZ			(63)

#elif defined(CONFIG_P4A_LL_DEBUG_UART2)
#define LL_DBG_UART_PHYS		P4A_UART2_PHYS
#define LL_DBG_UART_BASE		P4A_UART2_BASE

#define	LL_DBG_UMR_VALUE(clk, baud)	(((clk>>4)/baud) - 1)
#define LL_DBG_USR_TFLEVEL_SHIFT	(6)
#define LL_DBG_USR_TFLEVEL_MASK 	(0x3f<<LL_DBG_USR_TFLEVEL_SHIFT)
#define LL_DBG_UTX_FIFOSZ			(0x3e)

#elif defined(CONFIG_P4A_LL_DEBUG_UART1)
#define LL_DBG_UART_PHYS		P4A_UART1_PHYS
#define LL_DBG_UART_BASE		P4A_UART1_BASE

#define	LL_DBG_UMR_VALUE(clk, baud)	(((clk>>4)/baud) - 1)
#define LL_DBG_USR_TFLEVEL_SHIFT	(6)
#define LL_DBG_USR_TFLEVEL_MASK 	(0x3f<<LL_DBG_USR_TFLEVEL_SHIFT)
#define LL_DBG_UTX_FIFOSZ			(0x3e)

#endif

/*GPIO */
//GPIO
#define GPIO1_DIR_REG			    (PER_BASE_ADDRESS + (0x50<<2))
#define GPIO1_INPUT_REG 			(PER_BASE_ADDRESS + (0x51<<2)) 
#define GPIO1_OUTPUT_SET_REG	    (PER_BASE_ADDRESS + (0x52<<2)) 
#define GPIO1_OUTPUT_CLR_REG	    (PER_BASE_ADDRESS + (0x53<<2)) 
#define GPIO1_OUTPUT_REG			(PER_BASE_ADDRESS + (0x54<<2)) //R
#define GPIO1_EDGE_SEL		        (PER_BASE_ADDRESS + (0x40<<2))
#define GPIO1_INT_CLR				(PER_BASE_ADDRESS + (0x42<<2))
#define GPIO1_INT_STATUS			(PER_BASE_ADDRESS + (0x43<<2))
#if (CPU_ID==ARM2)
#define GPIO1_EDGE_INT_EN			(PER_BASE_ADDRESS + (0x44<<2))
#endif
#if (CPU_ID==ARM1)
#define GPIO1_EDGE_INT_EN		    (PER_BASE_ADDRESS + (0x41<<2))
#endif
#if (CPU_ID==M0)
#define GPIO1_EDGE_INT_EN		    (PER_BASE_ADDRESS + (0x45<<2))
#endif


#define GPIO2_DIR_REG				(PER_BASE_ADDRESS + (0x55<<2))
#define GPIO2_INPUT_REG 			(PER_BASE_ADDRESS + (0x56<<2))
#define GPIO2_OUTPUT_SET_REG	    (PER_BASE_ADDRESS + (0x57<<2))
#define GPIO2_OUTPUT_CLR_REG	    (PER_BASE_ADDRESS + (0x58<<2))
#define GPIO2_OUTPUT_REG			(PER_BASE_ADDRESS + (0x59<<2))
#define GPIO2_EDGE_SEL				(PER_BASE_ADDRESS + (0x5A<<2))
#if (CPU_ID==ARM2)
#define GPIO2_EDGE_INT_EN			(PER_BASE_ADDRESS + (0x5E<<2))
#endif
#if (CPU_ID==ARM1)
#define GPIO2_EDGE_INT_EN		    (PER_BASE_ADDRESS + (0x5B<<2))
#endif
#if (CPU_ID==M0)
#define GPIO2_EDGE_INT_EN		    (PER_BASE_ADDRESS + (0x5F<<2))
#endif

#define GPIO2_INT_CLR				(PER_BASE_ADDRESS + (0x5C<<2))
#define GPIO2_INT_STATUS			(PER_BASE_ADDRESS + (0x5D<<2))


#define GPIO3_DIR_REG               (PER_BASE_ADDRESS + (0x60<<2))
#define GPIO3_INPUT_REG             (PER_BASE_ADDRESS + (0x61<<2))
#define GPIO3_OUTPUT_SET_REG        (PER_BASE_ADDRESS + (0x62<<2))
#define GPIO3_OUTPUT_CLR_REG        (PER_BASE_ADDRESS + (0x63<<2))
#define GPIO3_OUTPUT_REG		    (PER_BASE_ADDRESS + (0x64<<2))
#define GPIO3_EDGE_SEL		        (PER_BASE_ADDRESS + (0x65<<2))
#define GPIO3_INT_CLR				(PER_BASE_ADDRESS + (0x67<<2))
#define GPIO3_INT_STATUS			(PER_BASE_ADDRESS + (0x68<<2))
#if (CPU_ID==ARM2)
#define GPIO3_EDGE_INT_EN			(PER_BASE_ADDRESS + (0x69<<2))
#endif
#if (CPU_ID==ARM1)
#define GPIO3_EDGE_INT_EN		    (PER_BASE_ADDRESS + (0x66<<2))
#endif
#if (CPU_ID==M0)
#define GPIO3_EDGE_INT_EN		    (PER_BASE_ADDRESS + (0x6A<<2))
#endif

#define GPIO4_DIR_REG               (PER_BASE_ADDRESS + (0x70<<2))
#define GPIO4_INPUT_REG             (PER_BASE_ADDRESS + (0x71<<2))
#define GPIO4_OUTPUT_SET_REG        (PER_BASE_ADDRESS + (0x72<<2))
#define GPIO4_OUTPUT_CLR_REG        (PER_BASE_ADDRESS + (0x73<<2))
#define GPIO4_OUTPUT_REG		    (PER_BASE_ADDRESS + (0x74<<2))
#define GPIO4_EDGE_SEL		        (PER_BASE_ADDRESS + (0x75<<2))
#define GPIO4_INT_CLR				(PER_BASE_ADDRESS + (0x77<<2))
#define GPIO4_INT_STATUS			(PER_BASE_ADDRESS + (0x78<<2))
#if (CPU_ID==ARM2)
#define GPIO4_EDGE_INT_EN			(PER_BASE_ADDRESS + (0x79<<2))
#endif
#if (CPU_ID==ARM1)
#define GPIO4_EDGE_INT_EN		    (PER_BASE_ADDRESS + (0x76<<2))
#endif
#if (CPU_ID==M0)
#define GPIO4_EDGE_INT_EN		    (PER_BASE_ADDRESS + (0x7A<<2)) 
#endif


#define GPIO5_DIR_REG               (PER_BASE_ADDRESS + (0xB0<<2))
#define GPIO5_INPUT_REG             (PER_BASE_ADDRESS + (0xB1<<2))
#define GPIO5_OUTPUT_SET_REG        (PER_BASE_ADDRESS + (0xB2<<2))
#define GPIO5_OUTPUT_CLR_REG        (PER_BASE_ADDRESS + (0xB3<<2))
#define GPIO5_OUTPUT_REG		    (PER_BASE_ADDRESS + (0xB4<<2))
#define GPIO5_EDGE_SEL		        (PER_BASE_ADDRESS + (0xB5<<2))

#define GPIO5_INT_CLR				(PER_BASE_ADDRESS + (0xB7<<2))
#define GPIO5_INT_STATUS			(PER_BASE_ADDRESS + (0xB8<<2))
#if (CPU_ID==ARM2)
#define GPIO5_EDGE_INT_EN			(PER_BASE_ADDRESS + (0xB9<<2))
#endif
#if (CPU_ID==ARM1)
#define GPIO5_EDGE_INT_EN		    (PER_BASE_ADDRESS + (0xB6<<2))
#endif
#if (CPU_ID==M0)
#define GPIO5_EDGE_INT_EN		    (PER_BASE_ADDRESS + (0xBA<<2)) 
#endif

#define GPIO6_DIR_REG               (PER_BASE_ADDRESS + (0xC0<<2))
#define GPIO6_INPUT_REG             (PER_BASE_ADDRESS + (0xC1<<2))
#define GPIO6_OUTPUT_SET_REG        (PER_BASE_ADDRESS + (0xC2<<2))
#define GPIO6_OUTPUT_CLR_REG        (PER_BASE_ADDRESS + (0xC3<<2))
#define GPIO6_OUTPUT_REG		    (PER_BASE_ADDRESS + (0xC4<<2))
#define GPIO6_EDGE_SEL		        (PER_BASE_ADDRESS + (0xC5<<2))
#define GPIO6_INT_CLR				(PER_BASE_ADDRESS + (0xC7<<2))
#define GPIO6_INT_STATUS			(PER_BASE_ADDRESS + (0xC8<<2))
#if (CPU_ID==ARM2)
#define GPIO6_EDGE_INT_EN			(PER_BASE_ADDRESS + (0xC9<<2))
#endif
#if (CPU_ID==ARM1)
#define GPIO6_EDGE_INT_EN		    (PER_BASE_ADDRESS + (0xC6<<2))
#endif
#if (CPU_ID==M0)
#define GPIO6_EDGE_INT_EN		    (PER_BASE_ADDRESS + (0xCA<<2))
#endif

#endif

