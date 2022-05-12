#pragma once

#include <cstdint>

#include "esp_attr.h"
#include "hal/gpio_types.h"

class TRIAC
{

public:
	explicit		TRIAC(gpio_num_t GPIOPinGate, gpio_num_t GPIOPinZeroCrossing);

	uint32_t		IRAM_ATTR TimerISR();
	void			IRAM_ATTR GpioISR();

	void 			setDuty(uint16_t value) { m_duty = value; m_initCycle = true; }

	void dump();


private:
	static void 	IRAM_ATTR TimerAdapter(void* triac);
	static void		IRAM_ATTR GpioAdapter(void* triac);

private:
	enum class DimmingMethods : uint8_t
	{
		TrailingEdge,
		LeadingEdge
	};

	int64_t m_lastZeroCrossing			= 0;
	int64_t m_onPeriodMs				= 0;
	int64_t m_offPeriodMs				= 0;
	int64_t m_mainsPeriodMs				= 0;

	uint16_t m_minPower					= 0;
	uint16_t m_duty						= 0;

	bool m_initCycle					= false;
	DimmingMethods m_method				= DimmingMethods::LeadingEdge;

	gpio_num_t m_GPIOPinGate			= GPIO_NUM_NC;
	gpio_num_t m_GPIOPinZeroCrossing	= GPIO_NUM_NC;
};
