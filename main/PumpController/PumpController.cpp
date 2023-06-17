#include "PumpController.hpp"

namespace
{
	constexpr float kDefaultKp = 100.0f;
	constexpr float kDefaultKi = 10.0f;
	constexpr float kDefaultKd = 1.0f;

	constexpr gpio_num_t kGPIOPin_TRIAC_Gate = GPIO_NUM_11;
	constexpr gpio_num_t kGPIOPin_TRIAC_ZC = GPIO_NUM_10;
}

PumpController::PumpController()
	: m_pid(&m_currentPressure, &m_pumpDuty, &m_targetPressure, kDefaultKp, kDefaultKi, kDefaultKd, QuickPID::Action::direct)
	, m_triac(kGPIOPin_TRIAC_Gate, kGPIOPin_TRIAC_ZC)
	, m_terms(kDefaultKp, kDefaultKi, kDefaultKd)
{
	m_pid.SetOutputLimits(20.0f, 100.0f);
	m_pid.SetMode(QuickPID::Control::automatic);
	m_pid.SetSampleTimeUs(100 * 1000);
	m_pid.SetProportionalMode(QuickPID::pMode::pOnError);
}

#include <cmath>
#include <algorithm>

static uint16_t calcDuty(float duty)
{
	constexpr float a = -0.0078125;
	constexpr float b = 1.802678571;
	constexpr float c = -2.321428571;

	const auto y = std::clamp<float>((a * pow(duty, 2.0f)) + (b * duty) + c, 0.0f, 100.0f);

	return static_cast<uint16_t>((y / 100) * 0xEFFF);
}

void PumpController::tick()
{
	if (m_inhibit || m_state != PumpState::Running)
		return;

	m_targetPressure = m_brewPressure;

	uint16_t duty;

	if (m_manualControl)
	{
		duty = calcDuty(m_pumpDuty);
	}
	else if (m_pid.Ready())
	{
		m_pid.Compute();
		m_averageDuty(m_pumpDuty);

		duty = calcDuty(m_averageDuty.get());
	}
	else
	{
		return;
	}

	if (duty < 0x1000)
		duty = 0;
	else
		duty += 0x1000;

	m_triac.setDuty(static_cast<int>(duty));
}

void PumpController::shutdown()
{
	m_inhibit = true;
	m_targetPressure = 0.0f;
	m_triac.setDuty(0);
}

void PumpController::updateCurrentPressure(const float pressure)
{
	m_averagePressure(pressure);

	m_currentPressure = m_averagePressure.get();
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

	m_pid.SetTunings(Kp * 0.01f, Ki * 0.01f, Kd * 0.01f);
}

void PumpController::setManualMode(bool enable)
{
	if (m_manualControl == enable)
		return;

	m_manualControl = enable;

	m_pid.SetMode(enable ? QuickPID::Control::manual : QuickPID::Control::automatic);
}

void PumpController::setManualDuty(float percent)
{
	if (m_pumpDuty == percent)
		return;

	if (! m_manualControl)
	{
		m_manualControl = true;
		m_pid.SetMode(QuickPID::Control::manual);
	}

	m_pumpDuty = percent;
}

void PumpController::start()
{
	if (m_state == PumpState::Running)
		return;

	if (m_inhibit)
		return;

	const auto& [Kp, Ki, Kd] = m_terms;
	m_pid.SetTunings(Kp * 0.1f, Ki * 0.1f, Kd * 0.1f);

	m_state = PumpState::Running;

	m_pid.SetMode(QuickPID::Control::automatic);
}

void PumpController::stop()
{
	if (m_state == PumpState::Stopped)
		return;

	m_state = PumpState::Stopped;

	m_targetPressure = 0.0f;

	auto pressureTemp = m_currentPressure;

	m_pumpDuty = 0.0f;
	m_currentPressure = 0.0f;
	m_pid.SetMode(QuickPID::Control::manual);

	m_currentPressure = pressureTemp;

	m_triac.setDuty(0);
}
