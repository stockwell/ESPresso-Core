#include "PressureEventLoop.hpp"

#include "Sensors/AnalogSensor.hpp"

namespace
{
	constexpr auto kPressurePollPeriodMs = 200;

	enum Events
	{
		PressurePollTimerElapsed,
	};
}

PressureEventLoop::PressureEventLoop()
	: EventLoop("PressureEvent")
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
		m_sensor->GetPressure();
		break;
	}
}
