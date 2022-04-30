#include "SSRController.hpp"

#include "driver/gpio.h"

namespace
{
	constexpr auto kCyclesMax = 50;
}

SSRController::SSRController(gpio_num_t GPIONum /*TODO: AC Period*/)
	: m_gpioPin(GPIONum)
{
	gpio_config_t gpioConfig = {};
	gpioConfig.intr_type = GPIO_INTR_DISABLE;
	gpioConfig.mode = GPIO_MODE_OUTPUT;
	gpioConfig.pull_up_en = GPIO_PULLUP_ENABLE;
	gpioConfig.pin_bit_mask = 1ULL << GPIONum;
	gpio_config(&gpioConfig);

	gpio_set_level(m_gpioPin, 0);
}


void SSRController::tick()
{
	// Turn off immediately
	if (m_nextOnCycles == 0 && m_currentOnCycles != 0)
	{
		gpio_set_level(m_gpioPin, 0);
		m_currentCycle = 0;
		m_currentOnCycles = 0;
		return;
	}

	++m_currentCycle;

	if (m_currentCycle > kCyclesMax)
	{
		gpio_set_level(m_gpioPin, 1);
		m_currentOnCycles = m_nextOnCycles;
		m_currentCycle = 0;
	}
	else if (m_currentCycle > m_currentOnCycles)
	{
		gpio_set_level(m_gpioPin, 0);
	}
}
