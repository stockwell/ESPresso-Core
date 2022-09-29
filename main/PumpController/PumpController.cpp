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
	m_pid.SetSampleTimeUs(50 * 1000);
	m_pid.SetProportionalMode(QuickPID::pMode::pOnMeas);
}

void PumpController::tick()
{
	if (m_inhibit || m_state != PumpState::Running)
		return;

	setPressureFromProfile();

	if (m_pid.Ready())
	{
		m_pid.Compute();
		m_averageDuty(m_pumpDuty);

		auto duty = static_cast<uint16_t>((m_averageDuty.get()/100) * 0xFFFF);

		if (duty < 0x1000)
			duty = 0;

		//printf("m_targetPressure = %0.2f / m_duty = %02x\n", m_targetPressure, duty);

		m_triac.setDuty(static_cast<int>(duty));
	}
}

void PumpController::shutdown()
{
	m_inhibit = true;
	m_targetPressure = 0.0f;
	m_triac.setDuty(0);
}

void PumpController::updateCurrentPressure(const float pressure)
{
	m_currentPressure = pressure;
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

	m_pid.SetTunings(Kp * 0.1f, Ki * 0.1f, Kd * 0.1f);
}

void PumpController::start()
{
	if (m_state == PumpState::Running)
		return;

	if (m_inhibit)
		return;

	const auto& [Kp, Ki, Kd] = m_terms;

	m_pid.SetTunings(Kp * 0.1f, Ki * 0.1f, Kd * 0.1f);

	m_startTime = std::chrono::system_clock::now();

	setPressureFromProfile();

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

void PumpController::setPressureFromProfile()
{
	using namespace std::chrono;

	auto elapsedTime = duration_cast<milliseconds>(system_clock::now() - m_startTime);

	// 0->8s pre-infusion
	// 8s->13s ramp up to full pressure

	if (elapsedTime < 8s)
	{
		m_targetPressure = 0.33f * m_brewPressure;
	}
	else if (elapsedTime < 13s)
	{
		m_targetPressure = 0.33f * m_brewPressure + ((elapsedTime.count() - 8000) / 5000.0f) * (m_brewPressure * 0.66f);
	}
	else if (elapsedTime < 30s)
	{
		m_targetPressure = m_brewPressure;
	}
	else
	{
		m_targetPressure = 0.0f;
	}
}
