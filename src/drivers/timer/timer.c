#include <types.h>
#include <common.h>
#include <io.h>
#include <hardware.h>
#include <timer.h>
#include <clk.h>

/*
 * Timer Registers Offset
 */
#define TLDR(x)         (0x00 + ((x) << 4))     // Timer Load Register
#define TCNTR(x)        (0x04 + ((x) << 4))     // Timer Count Register
#define TCR(x)          (0x08 + ((x) << 4))     // Timer Control Register
#define TISR(x)         (0x0C + ((x) << 4))     // Timer Interrupt Register
#define TPDR            (0x50)                  // Timer Prescale Divisor Register

static unsigned long timer_rate;

int timer_init(void)
{
/*
	int i;
	int clk_div_shift = 1;

	clock_switch(CLK_TIMER, 1);

	timer_rate = get_timer_clk() >> 1;

	// divide clock to 1Mhz around.
	for (i=2; i<=8; i++) {
		if ((timer_rate >> 1) < 1000000)
			break;

		timer_rate >>= 1;
		clk_div_shift = i;
	}

	SET_REG(TIMER_BASE_ADDR + TPDR, ((clk_div_shift - 1) << 0));

	// Timer 4 is use for free running clock
	SET_REG(TIMER_BASE_ADDR + TCR(4), 8);
	SET_REG(TIMER_BASE_ADDR + TISR(4), 1);
	SET_REG(TIMER_BASE_ADDR + TLDR(4), 0);
	SET_REG(TIMER_BASE_ADDR + TCR(4), 9);
*/
	return 0;
}

static void delay_cycles(unsigned long cycles)
{
/*
    unsigned long now, start;
    now = start = GET_REG(TIMER_BASE_ADDR + TCNTR(4));

    while ((start - now) < cycles)
        now = GET_REG(TIMER_BASE_ADDR + TCNTR(4));
*/
}

/* delay x useconds */
void __udelay(unsigned long usec)
{
   	unsigned long cycles;
	unsigned long rate_hz = timer_rate;
	unsigned long sec, msec;

	sec = usec / 1000000;
	msec = (usec / 1000) - (sec * 1000);
	usec = usec - (sec * 1000000) - (msec * 1000);

	/* the cycles needs for delay usec */
	cycles = (sec * rate_hz) + 
			(msec * (rate_hz / 1000)) + 
			(usec * (rate_hz / 1000) / 1000);
    //cycles = usec * (rate_hz / 1000 / 1000);        

    if (!cycles)
        cycles = 1;
	
	while (cycles) {
		unsigned long c = cycles > 0xffff ? 0xffff : cycles;

		delay_cycles(c);

		cycles -= c;
	}
}

