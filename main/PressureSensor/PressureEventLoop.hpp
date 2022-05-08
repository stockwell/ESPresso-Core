#pragma once

#include "BoilerEventLoop.hpp"
#include "PressureSensor.hpp"

#include "Lib/EventLoop.hpp"
#include "Lib/Timer.hpp"

#include "ADS1115.hpp"

class PressureEventLoop : public EventLoop
{
public:
	explicit PressureEventLoop();

protected:
	void eventHandler(int32_t eventId, void* data) override;

private:
	std::unique_ptr<Timer>				m_timer;
	std::unique_ptr<PressureSensor>		m_sensor;
};