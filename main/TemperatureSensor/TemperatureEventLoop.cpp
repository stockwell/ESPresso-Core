#include "TemperatureEventLoop.hpp"

#if CONFIG_BOILER_TEMP_SENSOR_MAX31865
	#include "TemperatureSensorMAX31865.hpp"
	using BoilerTemperatureSensor = TemperatureSensorMAX31865;
#endif

namespace
{
	constexpr auto kTemperaturePollPeriod = 50;

	enum Events
	{
		TemperaturePollTimerElapsed,

		Shutdown,
	};
}

void TemperatureEventLoop::shutdown()
{
	eventPost(Events::Shutdown);
}

TemperatureEventLoop::TemperatureEventLoop(BoilerEventLoop* boilerEventLoop)
	: EventLoop("TemperatureEvent")
	, m_boilerEventLoop(boilerEventLoop)
{
	m_sensor = std::make_unique<BoilerTemperatureSensor>();

	m_timer = std::make_unique<Timer>(kTemperaturePollPeriod, [this]() {
		eventPost(Events::TemperaturePollTimerElapsed);
	});

	m_timer->start();
}

void TemperatureEventLoop::eventHandler(int32_t eventId, void* data)
{
	switch (eventId)
	{
	case Events::TemperaturePollTimerElapsed:
		m_boilerEventLoop->setTemperature(BoilerEventLoop::CurrentBoilerTemp, m_sensor->GetTemperature());
		break;

	case Events::Shutdown:
		m_timer->stop();
		break;
	}
}
