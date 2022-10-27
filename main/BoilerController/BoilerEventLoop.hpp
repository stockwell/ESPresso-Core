#pragma once

#include "BoilerController.hpp"
#include "PumpEventLoop.hpp"

#include "Lib/EventLoop.hpp"
#include "Lib/Timer.hpp"

class BoilerEventLoop : public EventLoop
{
public:
	BoilerEventLoop(PumpEventLoop* pumpAPI);
	virtual ~BoilerEventLoop() = default;

	enum TemperatureTypes
	{
		CurrentBoilerTemp,
		CurrentTargetTemp,
		BrewTargetTemp,
		SteamTargetTemp,
	};

	void							setTemperature(TemperatureTypes type, float temperature);
	float							getTemperature(TemperatureTypes type);

	void							setPIDTerms(BoilerController::PIDTerms terms);
	BoilerController::PIDTerms		getPIDTerms();

	BoilerController::BoilerState	getState();

	void 							clearInhibit();
	void 							shutdown();

protected:
	void							eventHandler(int32_t eventId, void* data) override;

private:
	std::unique_ptr<Timer>			m_timer;

	BoilerController				m_controller;
};
