#include "driver/gpio.h"

class SSR
{
public:
	explicit SSR(gpio_num_t GPIONum);

	void update(uint16_t duty);
};
