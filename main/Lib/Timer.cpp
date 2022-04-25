#include "Timer.hpp"

Timer::Timer(uint32_t period, std::function<void()> cb, bool periodic)
	: m_cb(cb)
{
	m_timerHandle = xTimerCreate(  "Default",
		period / portTICK_PERIOD_MS,
		periodic ? pdTRUE : pdFALSE,
		this,
		callbackAdapter);

	if (! m_timerHandle)
		printf("Failed to reg timer\n");
}

Timer::~Timer()
{
	xTimerDelete(m_timerHandle, portMAX_DELAY);
}

void Timer::start()
{
	xTimerStart(m_timerHandle, portMAX_DELAY);
}

void Timer::stop()
{
	xTimerStop(m_timerHandle, portMAX_DELAY);
}

void Timer::setPeriod(uint32_t period)
{
	xTimerChangePeriod(m_timerHandle, period / portTICK_PERIOD_MS, portMAX_DELAY);
}

void Timer::elapsed()
{
	m_cb();
}

void Timer::callbackAdapter(TimerHandle_t xTimer)
{
	auto timer = static_cast<Timer*>(pvTimerGetTimerID(xTimer));

	timer->elapsed();
}
