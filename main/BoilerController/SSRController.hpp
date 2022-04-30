#include "driver/gpio.h"

class SSRController
{
public:
	explicit SSRController(gpio_num_t GPIONum);

	void setNextOnCycles(int cycles) { m_nextOnCycles = cycles; }

	void tick();

private:
	int m_nextOnCycles = 0;
	int m_currentOnCycles = 0;
	int m_currentCycle = 0;

	gpio_num_t m_gpioPin;
};
