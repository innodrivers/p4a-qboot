
#include <types.h>
#include <common.h>
#include <io.h>
#include <hardware.h>
#include <clk.h>

#ifdef CONFIG_P4A_FPGA
unsigned long get_uart_clk(int idx)
{
	return 38400000;
}
unsigned long get_timer_clk(void)
{
	return 38400000;
}
#else
static unsigned long sysin_clkrate = 19200000;

static unsigned long get_pll_clkrate(unsigned long pll_ctrl_reg_addr)
{
	unsigned long val;
	unsigned long NR, NF, NO;
	unsigned long rate;

	val = *REG32(pll_ctrl_reg_addr);

	NR = (val & 0x1f) + 1;
	NF = (((val >> 8) & 0x7f) + 1) * 2;
	NO = 1 << ((val >> 24) & 0x3);

	rate = sysin_clkrate * NF / (NR * NO);

	return rate;
}

static unsigned long get_peri_source_clkrate(void)
{
	int sel;
	unsigned long rate;

	sel = ((*REG32(PMU_CLKRST3_REG) >> 25)) & 0x3;
	switch (sel) {
	case 0:
		rate = get_pll_clkrate(PMU_PLL1_CTRL_REG);
		break;
	case 1:
		rate = get_pll_clkrate(PMU_PLL2_CTRL_REG);
		break;
	case 2:
		rate = get_pll_clkrate(PMU_PLL3_CTRL_REG);
		break;
	default:
		break;
	}

	return rate;
}

static unsigned long get_peri_ref_clkrate(void)
{
	unsigned long parent_rate;
	int div;
	unsigned long rate;

	parent_rate = get_peri_source_clkrate();
	div = ((*REG32(PMU_CLKRST2_REG) >> 25) & 0xf) + 1;

	rate = parent_rate/div;

	return rate;

}

unsigned long get_timer_clk(void)
{
	unsigned long parent_rate;
	int div;
	unsigned long rate;

	parent_rate = get_peri_ref_clkrate();
	div = (*REG32(GBL_CFG_PERI_CLK_REG) & 0xf) + 1;

	rate = parent_rate/div;

	return rate;
}

static unsigned long get_uart1_2_clkrate(void)
{
	return get_timer_clk();
}

static unsigned long get_axi_ref_clkrate(void)
{
	unsigned long rate, parent_rate;
	int sel;
	int div;

	sel = ((*REG32(PMU_CLKRST3_REG) >> 22)) & 0x7;
	switch (sel) {
	case 0:
		parent_rate = sysin_clkrate;
		break;
	case 0x4:
		parent_rate = get_pll_clkrate(PMU_PLL1_CTRL_REG);
		break;
	case 0x5:
		parent_rate = get_pll_clkrate(PMU_PLL2_CTRL_REG);
		break;
	case 0x6:
		parent_rate = get_pll_clkrate(PMU_PLL3_CTRL_REG);
		break;
	default:
		parent_rate = 0;
	}
	PRINT("%s sel %d, parent_rate %d\n", __FUNCTION__, sel, parent_rate);

	div = ((*REG32(PMU_CLKRST2_REG) >> 15) & 0x1f) + 1;

	rate = parent_rate/div;

	PRINT("%s div %d, rate %d\n", __FUNCTION__, div, rate);
	return rate;
}

static unsigned long get_hclk(void)
{
	unsigned long parent_rate;
	int div;
	unsigned long rate;

	parent_rate = get_axi_ref_clkrate();
	div = (*REG32(GBL_CFG_BUS_CLK_REG) & 0xf) + 1;

	rate = parent_rate / div;

	PRINT("%s div %d, rate %d\n", __FUNCTION__, div, rate);
	return rate;
}

static unsigned long get_uart4w_clkrate(void)
{
	return get_hclk();	
}

unsigned long get_uart_clk(int idx)
{
	if (idx == 0 || idx == 1)
	  return get_uart1_2_clkrate();

	if (idx == 2)
	  return get_uart4w_clkrate();

	return 0;
}

#endif


void clock_switch(enum clock_id clk, int on)
{
	switch (clk) {
	case CLK_TIMER:
		if (on)
			*REG32(GBL_CFG_BUS_CLK_REG) |= (0x1 << 15);
		else
			*REG32(GBL_CFG_BUS_CLK_REG) &= ~(0x1 << 15);
		break;
	case CLK_P4TIMER:
		break;
	case CLK_UART1:
	case CLK_UART2:
		if (on)
			*REG32(GBL_CFG_BUS_CLK_REG) |= (0x1 << 7);
		else
			*REG32(GBL_CFG_BUS_CLK_REG) &= ~(0x1 << 7);
		break;
	case CLK_UART4W:
		if (on) {
			*REG32(GBL_CFG_BUS_CLK_REG) |= (0x1 << 5);
			*REG32(GBL_CFG_SOFTRST_REG) |= (0x1 << 26);
		} else {
			*REG32(GBL_CFG_BUS_CLK_REG) &= ~(0x1 << 5);
			*REG32(GBL_CFG_SOFTRST_REG) &= ~(0x1 << 26);
		}
		break;
	case CLK_NAND:
		if (on) {
			*REG32(GBL_CFG_BUS_CLK_REG) |= (0x1 << 16);
			*REG32(GBL_CFG_SOFTRST_REG) |= (0x1 << 10);
		} else {
			*REG32(GBL_CFG_BUS_CLK_REG) &= ~(0x1 << 16);
			*REG32(GBL_CFG_SOFTRST_REG) &= ~(0x1 << 10);
		}
		break;
	default:
		break;
	}
}


