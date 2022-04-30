#include "driver/gpio.h"

class SSRController
{
public:
	explicit SSRController(gpio_num_t GPIONum);

	void tick(uint16_t duty);
};
