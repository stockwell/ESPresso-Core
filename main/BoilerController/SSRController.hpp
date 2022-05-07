#include "driver/gpio.h"

class SSRController
{
public:
	explicit SSRController(gpio_num_t GPIONum);

	void update(uint16_t duty);
};
