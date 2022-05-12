#pragma once

#include <set>
#include <memory>

#include "QuickPID.h"
#include "SSR.hpp"

class BoilerTemperatureDelegate
{
public:
	virtual void onBoilerCurrentTempChanged(float temp)	{ };
	virtual void onBoilerTargetTempChanged(float temp) { };
};

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

	void registerBoilerTemperatureDelegate(BoilerTemperatureDelegate* delegate);
	void deregisterBoilerTemperatureDelegate(BoilerTemperatureDelegate* delegate);

	void setBoilerTargetTemp(float temp);
	void setBoilerCurrentTemp(float temp);

	float getBoilerTargetTemp() const { return m_targetTemp; }
	float getBoilerCurrentTemp() const { return m_currentTemp; }

	void setPIDTerms(PIDTerms terms);
	PIDTerms getPIDTerms() const { return m_terms; }

	void tick();

private:
	std::set<BoilerTemperatureDelegate*> m_delegates;
	std::unique_ptr<QuickPID> m_pid;
	SSR m_ssr;

	float m_targetTemp		= 0.0;
	float m_currentTemp		= 0.0;
	float m_outputPower		= 0.0;

	PIDTerms m_terms;
};
