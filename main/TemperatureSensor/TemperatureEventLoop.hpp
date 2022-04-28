#pragma once

#include "BoilerEventLoop.hpp"
#include "TemperatureSensor.hpp"

#include "Lib/EventLoop.hpp"
#include "Lib/Timer.hpp"

class TemperatureEventLoop : public EventLoop
{
public:
	TemperatureEventLoop(BoilerEventLoop* boilerEventLoop);
	virtual ~TemperatureEventLoop() = default;

protected:
	void						EventHandler(int32_t eventId, void* data) override;

private:
	std::unique_ptr<Timer>				m_timer;
	std::unique_ptr<TemperatureSensor>	m_sensor;
	BoilerEventLoop*					m_boilerEventLoop;
};
