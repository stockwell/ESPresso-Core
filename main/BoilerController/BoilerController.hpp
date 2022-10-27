#pragma once

#include "PumpEventLoop.hpp"
#include "SSR.hpp"

#include "Lib/RollingAverage.hpp"

#include "QuickPID.h"

#include <chrono>
#include <memory>

class BoilerController
{
public:
	enum class BoilerState
	{
		Heating,
		Ready,
		Brewing,
		Inhibited,
	};

	using PIDTerms = std::tuple<float, float, float>;

	BoilerController(PumpEventLoop* pumpAPI);

	void updateCurrentTemp(float temp);
	void setBrewTargetTemp(float temp);
	void setSteamTargetTemp(float temp);

	float getTargetTemp() const			{ return m_targetTemp; }
	float getCurrentTemp() const		{ return m_currentTemp; }
	float getBrewTargetTemp() const		{ return m_brewTemp; }
	float getSteamTargetTemp() const	{ return m_steamTemp; }

	BoilerState getState() const		{ return m_state; }

	void setPIDTerms(PIDTerms terms);
	PIDTerms getPIDTerms() const 		{ return m_terms; }

	void clearInhibit()					{ m_inhibit = false; }

	void tick();
	void shutdown();

private:
	QuickPID			m_pid;
	SSR					m_ssr;

	const gpio_num_t	m_brewSwitchGPIO;
	const gpio_num_t	m_steamSwitchGPIO;

	RollingAverage<float, float, 10> m_averageTemp;

	float		m_brewTemp    = 0.0;
	float		m_steamTemp   = 0.0;

	float		m_targetTemp  = 0.0;
	float		m_currentTemp = 0.0;
	float		m_outputPower = 0.0;

	bool 		m_inhibit = false;

	BoilerState	m_state = BoilerState::Heating;
	PIDTerms	m_terms;

	PumpEventLoop* m_pumpAPI;

	std::chrono::time_point<std::chrono::steady_clock>	m_heatedTime;
};
