#include "PumpController.hpp"

namespace
{
	constexpr float kDefaultKp = 100.0f;
	constexpr float kDefaultKi = 10.0f;
	constexpr float kDefaultKd = 1.0f;

	constexpr gpio_num_t kGPIOPin_TRIAC_Gate = GPIO_NUM_46;
	constexpr gpio_num_t kGPIOPin_TRIAC_ZC = GPIO_NUM_3;
}

PumpController::PumpController()
	: m_pid(&m_currentPressure, &m_pumpDuty, &m_targetPressure, kDefaultKp, kDefaultKi, kDefaultKd, QuickPID::Action::direct)
	, m_triac(kGPIOPin_TRIAC_Gate, kGPIOPin_TRIAC_ZC)
	, m_terms(kDefaultKp, kDefaultKi, kDefaultKd)
{
	m_pid.SetOutputLimits(0, 1000);
	m_pid.SetMode(QuickPID::Control::automatic);
	m_pid.SetSampleTimeUs(200 * 1000);
}

void PumpController::tick()
{
	if (m_inhibit || m_state != PumpState::Running)
		return;

	if (m_pid.Compute())
		m_triac.setDuty(static_cast<int>(m_pumpDuty));

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

void PumpController::setPIDTerms(PIDTerms terms)
{
	if (terms == m_terms)
		return;

	m_terms = terms;

	auto [Kp, Ki, Kd] = terms;

	m_pid.SetTunings(Kp, Ki, Kd);
}

void PumpController::start()
{
	if (m_inhibit)
		return;

	m_targetPressure = m_brewPressure;

	m_state = PumpState::Running;

	m_pid.SetMode(QuickPID::Control::automatic);
}

void PumpController::stop()
{
	m_state = PumpState::Stopped;

	m_targetPressure = 0.0f;

	m_pid.SetMode(QuickPID::Control::manual);

	m_triac.setDuty(0);
}
