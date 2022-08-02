#pragma once

#include <cstdint>

#include "esp_attr.h"
#include "hal/gpio_types.h"

class TRIAC
{

public:
	explicit TRIAC(gpio_num_t GPIOPinGate, gpio_num_t GPIOPinZeroCrossing);

	void IRAM_ATTR GpioISR();

	void setDuty(uint16_t value) { if (value <= 500) m_duty = value / 10; }

private:
	static void		IRAM_ATTR GpioAdapter(void* triac);

private:

	int64_t m_lastZeroCrossing			= 0;
	int64_t m_mainsPeriodMs				= 0;

	uint16_t m_halfCycleCount			= 0;
	uint16_t m_onHalfCycles				= 0;
	uint16_t m_periodHalfCycles			= 50;
	uint16_t m_duty						= 0;

	gpio_num_t m_GPIOPinGate			= GPIO_NUM_NC;
	gpio_num_t m_GPIOPinZeroCrossing	= GPIO_NUM_NC;
};
