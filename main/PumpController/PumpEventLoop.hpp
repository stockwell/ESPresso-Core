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

	PumpController::PumpState getState();

	void	startPump();
	void	stopPump();
	void	shutdown();

protected:
	void						eventHandler(int32_t eventId, void* data) override;

private:
	std::unique_ptr<Timer>		m_timer;

	PumpController				m_controller;
};
