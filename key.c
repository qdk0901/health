
#include "common.h"

#define LONG_PRESS_CONDITION 3


int wait_for_key_change()
{
	int gpio_key = 0;
	int debounce = LONG_PRESS_CONDITION;
	do
	{
		read_gpio(GPIO_KEY, &gpio_key);
		sleep(1);
	}while(gpio_key == 1);
	
	do
	{
		read_gpio(GPIO_KEY, &gpio_key);
		debounce--;
		sleep(1);
	}while(gpio_key != 0 && debounce >= 0);
	
	if (debounce < 0)
		return KEY_CHANGE;
	
	return KEY_NOT_CHANGE;
}
