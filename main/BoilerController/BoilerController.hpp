#pragma once

#include <set>
#include <memory>

#include "QuickPID.h"
#include "SSR.hpp"

class BoilerController
{
public:
	enum class BoilerState
	{
		Heating,
		Ready,
	};

	using PIDTerms = std::tuple<float, float, float>;

	BoilerController();

	void updateCurrentTemp(float temp);
	void setBrewTargetTemp(float temp);
	void setSteamTargetTemp(float temp);

	float getTargetTemp() const			{ return m_targetTemp; }
	float getCurrentTemp() const		{ return m_currentTemp; }
	float getBrewTargetTemp() const		{ return m_brewTemp; }
	float getSteamTargetTemp() const	{ return m_steamTemp; }

	void setPIDTerms(PIDTerms terms);
	PIDTerms getPIDTerms() const 		{ return m_terms; }

	void tick();
	void shutdown();

private:
	QuickPID			m_pid;
	SSR					m_ssr;

	const gpio_num_t	m_brewSwitchGPIO;
	const gpio_num_t	m_steamSwitchGPIO;

	float		m_brewTemp    = 0.0;
	float		m_steamTemp   = 0.0;

	float		m_targetTemp  = 0.0;
	float		m_currentTemp = 0.0;
	float		m_outputPower = 0.0;

	bool 		m_inhibit = false;

	PIDTerms	m_terms;
};
