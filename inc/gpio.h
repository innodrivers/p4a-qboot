#ifndef _I10_GPIO_H_
#define _I10_GPIO_H_

extern int gpio_request(unsigned gpio);
extern int gpio_free(unsigned gpio);
extern int gpio_direction_output(unsigned gpio, int value);
extern int gpio_direction_input(unsigned gpio);
extern int gpio_set_value(unsigned gpio, int value);
extern int gpio_get_value(unsigned gpio);

#endif	// _I10_GPIO_H_
