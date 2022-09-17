#include <algorithm>
#include "Triac.hpp"
#include "esp_timer.h"
#include "driver/gpio.h"

namespace
{
	constexpr auto kGateEnableTime = 50;
}

uint32_t IRAM_ATTR TRIAC::TimerISR()
{
	auto now = esp_timer_get_time();

	// If no ZC signal received yet.
	if (m_lastZeroCrossing == 0)
		return 0;

	auto time_since_zc = now - m_lastZeroCrossing;
	if (m_duty == 0xFFFF || m_duty == 0)
		return 0;

	if (m_onPeriodMs != 0 && time_since_zc >= m_onPeriodMs)
	{
		m_onPeriodMs = 0;
		gpio_set_level(m_GPIOPinGate, 1);

		// Prevent too short pulses
		m_offPeriodMs = std::max(m_offPeriodMs, time_since_zc + kGateEnableTime);
	}

	if (m_offPeriodMs != 0 && time_since_zc >= m_offPeriodMs)
	{
		m_offPeriodMs = 0;
		gpio_set_level(m_GPIOPinGate, 0);
	}

	if (time_since_zc < m_onPeriodMs)
		return m_onPeriodMs - time_since_zc;
	else if (time_since_zc < m_offPeriodMs)
		return m_offPeriodMs - time_since_zc;

	// Already past last cycle time, schedule next call shortly
	if (time_since_zc >= m_mainsPeriodMs)
		return 100;

	return m_mainsPeriodMs - time_since_zc;
}

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
	}

	if (m_duty == 0xFFFF)
	{
		// fully on, enable output immediately
		gpio_set_level(m_GPIOPinGate, 1);
	}
	else if (m_initCycle)
	{
		// send a full cycle
		m_initCycle = false;
		m_onPeriodMs = 0;
		m_offPeriodMs = m_mainsPeriodMs;
	}
	else if (m_duty == 0)
	{
		// fully off, disable output immediately
		gpio_set_level(m_GPIOPinGate, 0);
	}
	else
	{
		if (m_method == DimmingMethods::TrailingEdge)
		{
			m_onPeriodMs = 1;  // cannot be 0
			m_offPeriodMs = std::max<int64_t>(10, (m_duty * m_mainsPeriodMs) / 0xFFFF);
		}
		else
		{
			// calculate time until enable in µs: (1.0-value)*cycle_time, but with integer arithmetic
			// also take into account min_power
			auto min_us = m_mainsPeriodMs * m_minPower / 1000;
			m_onPeriodMs= std::max<int64_t>(1, ((0xFFFF - m_duty) * (m_mainsPeriodMs - min_us)) / 0xFFFF);

			if (m_method == DimmingMethods::LeadingPulse)
			{
				// Minimum pulse time should be enough for the TRIAC to trigger when it is close to the ZC zone
				// this is for brightness near 99%
				m_offPeriodMs = std::max(m_onPeriodMs + kGateEnableTime, m_mainsPeriodMs / 10);
			}
			else
			{
				gpio_set_level(m_GPIOPinGate, 0);
				m_offPeriodMs = m_mainsPeriodMs;
			}
		}
	}
}

void IRAM_ATTR TRIAC::TimerAdapter(void* triac)
{
	if (triac == nullptr)
		return;

	static_cast<TRIAC*>(triac)->TimerISR();
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
	esp_timer_create_args_t create_args =
	{
		.callback = TRIAC::TimerAdapter,
		.arg = (void*)this,
		.name = "TRIAC"
	};

	esp_timer_handle_t timerHandle;
	if (auto err = esp_timer_create(&create_args, &timerHandle); err != 0)
	{
		printf("Fatal: Failed to create TRIAC timer!\n");
		abort();
	}

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

	esp_timer_start_periodic(timerHandle, 2);
}
