#include "PumpController.hpp"

namespace
{
	constexpr float kDefaultKp = 1.0f;
	constexpr float kDefaultKi = 1.0f;
	constexpr float kDefaultKd = 1.0f;

	constexpr gpio_num_t kGPIOPin_TRIAC_Gate = GPIO_NUM_6;
	constexpr gpio_num_t kGPIOPin_TRIAC_ZC = GPIO_NUM_7;
}

PumpController::PumpController()
	: m_pid(&m_currentPressure, &m_pumpDuty, &m_targetPressure, kDefaultKp, kDefaultKi, kDefaultKd, QuickPID::Action::direct)
	, m_triac(kGPIOPin_TRIAC_Gate, kGPIOPin_TRIAC_ZC)
{
	m_pid.SetOutputLimits(0, 100);
	m_pid.SetMode(QuickPID::Control::automatic);
}

void PumpController::tick()
{
	m_pid.Compute();
}

void PumpController::shutdown()
{
	m_inhibit = true;
	m_targetPressure = 0.0f;
	m_triac.setDuty(0);
}

void PumpController::setBrewPressure(const float pressure)
{
	m_brewPressure = pressure;
}

void PumpController::start()
{
	if (m_inhibit)
		return;

	m_targetPressure = m_brewPressure;
}

void PumpController::stop()
{
	m_targetPressure = 0.0f;
}
