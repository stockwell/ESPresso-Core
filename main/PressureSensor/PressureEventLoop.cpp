#include "PressureEventLoop.hpp"

#include "Sensors/AnalogSensor.hpp"

#include <future>

namespace
{
	constexpr auto kPressurePollPeriodMs = 200;

	enum Events
	{
		PressurePollTimerElapsed,

		GetPressure,
	};
}

float PressureEventLoop::getPressure()
{
	auto prom = new std::promise<float>();
	std::future<float> fut = prom->get_future();

	eventPost(Events::GetPressure, sizeof(void*), &prom);

	fut.wait();
	return fut.get();
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
		m_pressure = m_sensor->GetPressure();
		break;

	case Events::GetPressure:
		auto* prom = static_cast<std::promise<float>**>(data);

		(*prom)->set_value(m_pressure);
		delete *prom;

		break;
		break;
	}
}
