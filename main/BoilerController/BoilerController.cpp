#include "BoilerController.hpp"

namespace
{
	constexpr float kDefaultKp = 500.0f;
	constexpr float kDefaultKi = 500.0f;
	constexpr float kDefaultKd = 1.0f;

	constexpr int kGPIOPin_SSR = 23;
}

BoilerController::BoilerController()
	: m_ssrController(static_cast<gpio_num_t>(kGPIOPin_SSR))
	, m_terms(kDefaultKp, kDefaultKi, kDefaultKd)
{
	auto [Kp, Ki, Kd] = m_terms;

	m_pid = std::make_unique<QuickPID>(&m_currentTemp, &m_outputPower, &m_targetTemp, Kp, Ki, Kd, QuickPID::Action::direct);

	m_pid->SetSampleTimeUs(10000);
	m_pid->SetOutputLimits(0, 1024);
	m_pid->SetMode(QuickPID::Control::automatic);
}

void BoilerController::registerBoilerTemperatureDelegate(BoilerTemperatureDelegate* delegate)
{
	if (m_delegates.find(delegate) != m_delegates.end())
		return;

	m_delegates.emplace(delegate);
}

void BoilerController::deregisterBoilerTemperatureDelegate(BoilerTemperatureDelegate* delegate)
{
	if (auto it = m_delegates.find(delegate); it != m_delegates.end())
		m_delegates.erase(it);
}

void BoilerController::setBoilerCurrentTemp(float temp)
{
	if (m_currentTemp == temp)
		return;

	m_currentTemp = temp;

	for (auto delegate: m_delegates)
		delegate->onBoilerCurrentTempChanged(temp);
}

void BoilerController::setBoilerTargetTemp(float temp)
{
	if (m_targetTemp == temp)
		return;

	m_targetTemp = temp;

	for (const auto delegate: m_delegates)
		delegate->onBoilerTargetTempChanged(temp);
}

void BoilerController::setPIDTerms(PIDTerms terms)
{
	if (terms == m_terms)
		return;

	auto [Kp, Ki, Kd] = terms;

	m_pid->SetTunings(Kp, Ki, Kd);
}

void BoilerController::tick()
{
	m_pid->Compute();

	m_ssrController.tick(static_cast<int>(m_outputPower));
}
