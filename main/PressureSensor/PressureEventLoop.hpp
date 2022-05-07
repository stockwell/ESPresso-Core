#pragma once

#include "BoilerEventLoop.hpp"
#include "PressureSensor.hpp"

#include "Lib/EventLoop.hpp"
#include "Lib/Timer.hpp"

class PressureEventLoop : public EventLoop
{
public:
	explicit PressureEventLoop(BoilerEventLoop* boilerEventLoop);

protected:
	void eventHandler(int32_t eventId, void* data) override;

private:
	std::unique_ptr<Timer>				m_timer;
	std::unique_ptr<PressureSensor>		m_sensor;
	BoilerEventLoop*					m_boilerEventLoop;
};
