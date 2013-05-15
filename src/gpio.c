#include <types.h>
#include <common.h>
#include <io.h>
#include <hardware.h>

static uint32_t gpio_bank_state[6] = {0};
//static enum clock_id gpio_bank_clkid[6] = { CLK_GPIO0, CLK_GPIO1, CLK_GPIO2, CLK_GPIO3, CLK_GPIO4, CLK_GPIO5};
#define GPIO_GROUP_MEM_NUM (32)
#define GPIO_bit(x)      (1L << x)
int gpio_get_value(unsigned gpio)
{
	
	unsigned int gbank=0;
	unsigned int gpin=0;
	int ret = -1;

	if(gpio > GPIO_GROUP_MEM_NUM*6)
	{
	   return -1;
	}
	
	gbank= gpio/GPIO_GROUP_MEM_NUM + 1;
	gpin= gpio%GPIO_GROUP_MEM_NUM;
	
	switch (gbank) {
		case 1:  
				ret =  ( GET_REG(GPIO1_INPUT_REG) & GPIO_bit(gpin)) ? 1 : 0;
				break;
		case 2:
				ret =  ( GET_REG(GPIO2_INPUT_REG) & GPIO_bit(gpin)) ? 1 : 0;
				break;
		case 3:
				ret =  ( GET_REG(GPIO3_INPUT_REG) & GPIO_bit(gpin)) ? 1 : 0;
				break;
		case 4:
				ret =  ( GET_REG(GPIO4_INPUT_REG) & GPIO_bit(gpin)) ? 1 : 0;
				break;
		case 5:
				ret =  ( GET_REG(GPIO5_INPUT_REG) & GPIO_bit(gpin)) ? 1 : 0;
				break;
		case 6:
				ret =  ( GET_REG(GPIO6_INPUT_REG) & GPIO_bit(gpin)) ? 1 : 0;
				break;
		default:
				ret = -1;
				break;
	}
	
	return ret;
}

int gpio_set_value(unsigned gpio, int value)
{
	unsigned int  gbank=0;
	unsigned int  gpin=0;
	int ret = 0;
	if(gpio > GPIO_GROUP_MEM_NUM*6){
	   	return -1;
	}
	
	gbank =  gpio/GPIO_GROUP_MEM_NUM+1;
	gpin = gpio%GPIO_GROUP_MEM_NUM;

	switch (gbank) {
		case 1:  
				 if(value) {
				            SET_REG (GPIO1_OUTPUT_SET_REG,(GET_REG(GPIO1_OUTPUT_SET_REG) | GPIO_bit(gpin)));
				 }
				 else  {
				           SET_REG (GPIO1_OUTPUT_CLR_REG,(GET_REG(GPIO1_OUTPUT_CLR_REG) | GPIO_bit(gpin)));
				 }
				break;
		case 2:
				if(value) {
				            SET_REG (GPIO2_OUTPUT_SET_REG,(GET_REG(GPIO2_OUTPUT_SET_REG) | GPIO_bit(gpin)));
				 }
				 else  {
				           SET_REG (GPIO2_OUTPUT_CLR_REG,(GET_REG(GPIO2_OUTPUT_CLR_REG) | GPIO_bit(gpin)));
				 }
				break;
		case 3:
				if(value) {
				            SET_REG (GPIO3_OUTPUT_SET_REG,(GET_REG(GPIO3_OUTPUT_SET_REG) | GPIO_bit(gpin)));
				 }
				 else  {
				           SET_REG (GPIO3_OUTPUT_CLR_REG,(GET_REG(GPIO3_OUTPUT_CLR_REG) | GPIO_bit(gpin)));
				 }
				break;
		case 4:
				if(value) {
				            SET_REG (GPIO4_OUTPUT_SET_REG,(GET_REG(GPIO4_OUTPUT_SET_REG) | GPIO_bit(gpin)));
				 }
				 else  {
				           SET_REG (GPIO4_OUTPUT_CLR_REG,(GET_REG(GPIO4_OUTPUT_CLR_REG) | GPIO_bit(gpin)));
				 }
				break;
		case 5:
				if(value) {
				            SET_REG (GPIO5_OUTPUT_SET_REG,(GET_REG(GPIO5_OUTPUT_SET_REG) | GPIO_bit(gpin)));
				 }
				 else  {
				           SET_REG (GPIO5_OUTPUT_CLR_REG,(GET_REG(GPIO5_OUTPUT_CLR_REG) | GPIO_bit(gpin)));
				 }
				break;
		case 6:
				if(value) {
				            SET_REG (GPIO6_OUTPUT_SET_REG,(GET_REG(GPIO6_OUTPUT_SET_REG) | GPIO_bit(gpin)));
				 }
				 else  {
				           SET_REG (GPIO6_OUTPUT_CLR_REG,(GET_REG(GPIO6_OUTPUT_CLR_REG) | GPIO_bit(gpin)));
				 }
				break;
		default:
				ret = -1;
				break;
	}
	
	return ret;
	
}

static int set_gpio_direction(unsigned gpio, int output)
{
	unsigned int  gbank = 0;
	unsigned int gpin = 0;
	int ret = 0;

	if(gpio > GPIO_GROUP_MEM_NUM*6){
			return -1;
	}
	
	gbank= gpio/GPIO_GROUP_MEM_NUM+1;
	gpin= gpio%GPIO_GROUP_MEM_NUM;
	
	switch (gbank) {
		case 1:  
				 if(output) { /*set direction output */
				            SET_REG (GPIO1_DIR_REG,(GET_REG(GPIO1_DIR_REG) & ~ GPIO_bit(gpin)));
				 }
				 else  { /*set direction input */
				           SET_REG (GPIO1_DIR_REG,(GET_REG(GPIO1_DIR_REG) | GPIO_bit(gpin)));
				 }
				break;
		case 2:
				if(output) {
				            SET_REG (GPIO2_DIR_REG,(GET_REG(GPIO2_DIR_REG) & ~ GPIO_bit(gpin)));
				 }
				 else  {
				           SET_REG (GPIO2_DIR_REG,(GET_REG(GPIO2_DIR_REG) | GPIO_bit(gpin)));
				 }
				break;
		case 3:
				if(output) {
				            SET_REG (GPIO3_DIR_REG,(GET_REG(GPIO3_DIR_REG) & ~ GPIO_bit(gpin)));
				 }
				 else  {
				           SET_REG (GPIO3_DIR_REG,(GET_REG(GPIO3_DIR_REG) | GPIO_bit(gpin)));
				 }
				break;
		case 4:
				if(output) {
				            SET_REG (GPIO4_DIR_REG,(GET_REG(GPIO4_DIR_REG)& ~ GPIO_bit(gpin)));
				 }
				 else  {
				           SET_REG (GPIO4_DIR_REG,(GET_REG(GPIO4_DIR_REG) | GPIO_bit(gpin)));
				 }
				break;
		case 5:
				if(output) {
				            SET_REG (GPIO5_DIR_REG,(GET_REG(GPIO5_DIR_REG) & ~ GPIO_bit(gpin)));
				 }
				 else  {
				           SET_REG (GPIO5_DIR_REG,(GET_REG(GPIO5_DIR_REG) | GPIO_bit(gpin)));
				 }
				break;
		case 6:
				if(output) {
				            SET_REG (GPIO6_DIR_REG,(GET_REG(GPIO6_DIR_REG)& ~ GPIO_bit(gpin)));
				 }
				 else  {
				           SET_REG (GPIO6_DIR_REG,(GET_REG(GPIO6_DIR_REG) | GPIO_bit(gpin)));
				 }
				break;
		default:
				ret = -1;
				break;
	}
	
	return ret;
	
}

int gpio_direction_input(unsigned gpio)
{
	set_gpio_direction(gpio, 0);

	return 0;
}

int gpio_direction_output(unsigned gpio, int value)
{
	gpio_set_value(gpio, value);

	set_gpio_direction(gpio, 1);

	return 0;
}

int gpio_request(unsigned gpio)
{
	unsigned bank = gpio >> 5;

//	if (!gpio_bank_state[bank]) {
//		clock_switch(gpio_bank_clkid[bank], 1);
//	}

	gpio_bank_state[bank] |= GPIO_bit(gpio & 0x1f);	
	return 0;
}

int gpio_free(unsigned gpio)
{
	unsigned bank = gpio >> 5;

	gpio_bank_state[bank] &= ~ GPIO_bit(gpio & 0x1f);

//	if (!gpio_bank_state[bank])
//		clock_switch(gpio_bank_clkid[bank], 0);

	return 0;
}
