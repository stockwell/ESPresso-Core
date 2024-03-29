#pragma once

#include "PumpController.hpp"

#include <memory>

#include "Lib/EventLoop.hpp"
#include "Lib/Timer.hpp"

class PumpEventLoop : private EventLoop
{
public:
	enum PressureTypes
	{
		CurrentPressure,
		TargetPressure,
		BrewTargetPressure,
	};

	PumpEventLoop();
	virtual ~PumpEventLoop() = default;

	void	setPressure(PressureTypes type, float pressure);
	float	getPressure(PressureTypes type);

	void	setManualDuty(float percent);
	float	getManualDuty();

	void	setManualMode(bool enable);
	bool	getManualMode();

	PumpController::PumpState getState();
	PumpController::PIDTerms  getPIDTerms();

	void setPIDTerms(PumpController::PIDTerms);

	void	startPump();
	void	stopPump();
	void	shutdown();

protected:
	void						eventHandler(int32_t eventId, void* data) override;

private:
	std::unique_ptr<Timer>		m_timer;

	PumpController				m_controller;
};
