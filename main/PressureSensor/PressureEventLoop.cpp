#include "PressureEventLoop.hpp"

#include "Sensors/AnalogSensor.hpp"

#include <future>

namespace
{
	constexpr auto kPressurePollPeriodMs = 200;

	enum Events
	{
		PressurePollTimerElapsed,

		Shutdown,
	};
}

void PressureEventLoop::shutdown()
{
	eventPost(Events::Shutdown);
}

PressureEventLoop::PressureEventLoop(PumpEventLoop* pumpAPI)
	: EventLoop("PressureEvent")
	, m_pumpAPI(pumpAPI)
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
		m_pressure = m_sensor->GetPressure();

		m_pumpAPI->setPressure(PumpEventLoop::CurrentPressure, m_pressure);
		break;

	case Events::Shutdown:
		m_timer->stop();
		break;
	}
}
