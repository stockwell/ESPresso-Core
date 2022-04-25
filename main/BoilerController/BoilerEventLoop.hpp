#pragma once

#include "BoilerController.hpp"

#include "Lib/EventLoop.hpp"
#include "Lib/Timer.hpp"

class BoilerEventLoop : public EventLoop
{
public:
	BoilerEventLoop();
	virtual ~BoilerEventLoop() = default;

	void						setBoilerTemperature(float temperature);
	float						getBoilerTemperature();

	void						setPIDTerms(BoilerController::PIDTerms terms);
	BoilerController::PIDTerms	getPIDTerms();

protected:
	void						EventHandler(int32_t eventId, void* data) override;

private:
	std::unique_ptr<Timer>		m_timer;

	BoilerController			m_controller;
};
