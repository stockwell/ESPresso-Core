#pragma once

#include <memory>

#include "QuickPID.h"
#include "Triac.hpp"

class PumpController
{
public:
	using PIDTerms = std::tuple<float, float, float>;

	PumpController();

	void tick();

	void start();
	void stop();

	void shutdown();

	void updateCurrentPressure(const float pressure) { m_currentPressure = pressure; }
	void setBrewPressure(const float pressure);

	float getCurrentPressure() const { return m_currentPressure; }
	float getTargetPressure() const { return m_targetPressure; }
	float getBrewPressure() const { return m_brewPressure; }

private:
	QuickPID	m_pid;
	TRIAC		m_triac;

	float		m_currentPressure	= 0.0f;
	float		m_targetPressure	= 0.0f;
	float 		m_brewPressure		= 9.0f;

	float 		m_pumpDuty			= 0.0f;

	bool 		m_inhibit 			= false;
};
