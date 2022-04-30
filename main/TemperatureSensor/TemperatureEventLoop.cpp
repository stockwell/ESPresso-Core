#include "TemperatureEventLoop.hpp"

#include "TemperatureSensorMAX31865.hpp"

namespace
{
	constexpr auto kTemperaturePollPeriod = 10;

	enum Events
	{
		TemperaturePollTimerElapsed,
	};
}

TemperatureEventLoop::TemperatureEventLoop(BoilerEventLoop* boilerEventLoop)
	: EventLoop("TemperatureEvent")
	, m_boilerEventLoop(boilerEventLoop)
{
	m_sensor = std::make_unique<TemperatureSensorMAX31865>();

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
		m_boilerEventLoop->updateBoilerTemperature(m_sensor->GetTemperature());
		break;
	}
}
