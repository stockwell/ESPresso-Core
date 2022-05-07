#include "PressureEventLoop.hpp"

#include "Sensors/AnalogSensor.hpp"

namespace
{
	constexpr auto kPressurePollPeriodMs = 50;

	enum Events
	{
		PressurePollTimerElapsed,
	};
}

PressureEventLoop::PressureEventLoop(BoilerEventLoop* boilerEventLoop)
	: EventLoop("PressureEvent")
	, m_boilerEventLoop(boilerEventLoop)
{
	m_sensor = std::make_unique<AnalogSensor>();

	m_timer = std::make_unique<Timer>(kPressurePollPeriodMs, [this]() {
		eventPost(Events::PressurePollTimerElapsed);
	});

	m_timer->start();
}

void PressureEventLoop::eventHandler(int32_t eventId, void* data)
{
	switch (eventId)
	{
	case Events::PressurePollTimerElapsed:
		// Update pressure in pump controller?
		break;
	}
}
