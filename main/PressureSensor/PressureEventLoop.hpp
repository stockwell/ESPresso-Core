#pragma once

#include "BoilerEventLoop.hpp"
#include "PressureSensor.hpp"

#include "Lib/EventLoop.hpp"
#include "Lib/Timer.hpp"

#include "ADS1115.hpp"

#include "Triac.hpp"

class PressureEventLoop : public EventLoop
{
public:
	explicit PressureEventLoop();

	void setTRIACDuty(uint16_t value);

protected:
	void eventHandler(int32_t eventId, void* data) override;

private:
	std::unique_ptr<Timer>				m_timer;
	std::unique_ptr<PressureSensor>		m_sensor;
	std::unique_ptr<TRIAC>				m_TRIAC;
};
