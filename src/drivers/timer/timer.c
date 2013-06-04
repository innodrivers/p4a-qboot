#include <types.h>
#include <common.h>
#include <io.h>
#include <hardware.h>
#include <timer.h>
#include <clk.h>

/*
 * Timer Registers Offset
 */
//#define TLDR(x)         (0x00 + ((x) << 4))     // Timer Load Register
//#define TCNTR(x)        (0x04 + ((x) << 4))     // Timer Count Register
//#define TCR(x)          (0x08 + ((x) << 4))     // Timer Control Register
//#define TISR(x)         (0x0C + ((x) << 4))     // Timer Interrupt Register
//#define TPDR            (0x50)                  // Timer Prescale Divisor Register
#define P4A_TIMER2_MACH_REG0  (P4A_TIMER2_PHYS+(0<<2))
#define P4A_TIMER2_MACH_REG1  (P4A_TIMER2_PHYS+(1<<2))
#define P4A_TIMER2_CTRL_REG	(P4A_TIMER2_PHYS+(2<<2))
#define P4A_TIMER2_CNT_REG		(P4A_TIMER2_PHYS+(3<<2))

#define TIME_ENABLE				(1<<0)
#define TIME_RESETMR0_ENABLE	(0x1<<1)
#define TIME_RESETMR1_ENABLE	(0x1<<2)
#define TIME_RESETCNT 			(1<<3)

#define TIME_RESET_MACH ()

//#define TIMER2_TUS(x)  ((102400000/1000000)*x -1)
static unsigned long timer_rate;

int timer_init(void)
{
	unsigned int ctrl;
	SET_REG(P4A_TIMER2_CTRL_REG,0); 	//dis
	SET_REG(P4A_TIMER2_CTRL_REG,TIME_RESETCNT|TIME_ENABLE);
	timer_rate = 102400000;

	return 0;
}

static void delay_cycles(unsigned long cycles)
{

    unsigned long now, start;
    SET_REG(P4A_TIMER2_CTRL_REG,GET_REG(P4A_TIMER2_CTRL_REG)|TIME_RESETCNT);
    now = start = GET_REG(P4A_TIMER2_CNT_REG);

    while ((now - start) < cycles)
        now = GET_REG(P4A_TIMER2_CNT_REG);
}

/* delay x useconds */
void __udelay(unsigned long usec)
{
   	unsigned long cycles;
	unsigned long rate_hz = timer_rate;
	unsigned long sec, msec;

	if(0 == usec)
		return;
	
#if 0
	sec = usec / 1000000;
	msec = (usec / 1000) - (sec * 1000);
	usec = usec - (sec * 1000000) - (msec * 1000);

	/* the cycles needs for delay usec */
	cycles = (sec * rate_hz) + 
			(msec * (rate_hz / 1000)) + 
			(usec * (rate_hz / 1000) / 1000);
#endif
    cycles = usec * (rate_hz / 1000 / 1000);

    if (!cycles)
        cycles = 1;
	
	while (cycles) {
		unsigned long c = cycles > 0xffff ? 0xffff : cycles;

		delay_cycles(c);

		cycles -= c;
	}
}

