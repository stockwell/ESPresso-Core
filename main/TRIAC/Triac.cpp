#include "Triac.hpp"

#include "esp_timer.h"
#include "driver/gpio.h"

void IRAM_ATTR TRIAC::GpioISR()
{
	auto prev_crossed = m_lastZeroCrossing;

	// 50Hz mains frequency should give a half cycle of 10ms a 60Hz will give 8.33ms
	// in any case the cycle last at least 5ms
	m_lastZeroCrossing = esp_timer_get_time();
	auto cycle_time = m_lastZeroCrossing - prev_crossed;

	if (cycle_time > 5000)
	{
		m_mainsPeriodMs = cycle_time;
	}
	else
	{
		// Otherwise, this is noise and this is 2nd (or 3rd...) fall in the same pulse
		// Consider this is the right fall edge and accumulate the cycle time instead
		m_mainsPeriodMs += cycle_time;
		return;
	}

	m_halfCycleCount++;

	if (++m_halfCycleCount >= m_periodHalfCycles)
	{
		m_halfCycleCount = 0;

		if (m_duty != 0)
		{
			gpio_set_level(m_GPIOPinGate, 1);
			m_halfCycleCount = m_duty;
		}
	}
	else if (m_halfCycleCount >= m_onHalfCycles)
	{
		gpio_set_level(m_GPIOPinGate, 0);
	}
}

void IRAM_ATTR TRIAC::GpioAdapter(void* triac)
{
	if (triac == nullptr)
		return;

	static_cast<TRIAC*>(triac)->GpioISR();
}

TRIAC::TRIAC(gpio_num_t GPIOPinGate, gpio_num_t GPIOPinZeroCrossing)
	: m_duty(0x0)
	, m_GPIOPinGate(GPIOPinGate)
	, m_GPIOPinZeroCrossing(GPIOPinZeroCrossing)
{
	gpio_config_t gpioConfig = {};

	// Init TRIAC Gate Pin
	gpioConfig.pin_bit_mask = 1ULL << GPIOPinGate,
	gpioConfig.mode = GPIO_MODE_OUTPUT,
	gpio_config(&gpioConfig);
	gpio_set_level(GPIOPinGate, 1);

	// Init Zero Crossing Pin
	gpioConfig.pin_bit_mask = 1ULL << m_GPIOPinZeroCrossing;
	gpioConfig.mode = GPIO_MODE_INPUT;
	gpioConfig.intr_type = GPIO_INTR_NEGEDGE;
	gpioConfig.pull_up_en = GPIO_PULLUP_ENABLE;
	gpio_config(&gpioConfig);

	gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
	gpio_isr_handler_add(static_cast<gpio_num_t>(m_GPIOPinZeroCrossing), &TRIAC::GpioAdapter, (void*)this);
}
