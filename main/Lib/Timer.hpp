#pragma once

#include <functional>

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

class Timer
{
public:
	Timer(uint32_t period, std::function<void()> cb, bool periodic = true);
	~Timer();

	void start();
	void stop();
	void setPeriod(uint32_t period);

	void elapsed();

private:
	static void callbackAdapter(TimerHandle_t xTimer);

private:
	std::function<void()>	m_cb;
	TimerHandle_t			m_timerHandle;
};
