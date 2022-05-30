#include "BoilerController.hpp"

namespace
{
	constexpr float kDefaultKp = 500.0f;
	constexpr float kDefaultKi = 500.0f;
	constexpr float kDefaultKd = 1.0f;

	constexpr int kGPIOPin_SSR = 18;
	constexpr int kGPIOPin_BrewSwitch = 19;
	constexpr int kGPIOPin_SteamSwitch = 20;

	constexpr auto kBrewTempMax = 100.0f;
	constexpr auto kShutdownTemp = 160.0f;
}

BoilerController::BoilerController()
	: m_pid(&m_currentTemp, &m_outputPower, &m_targetTemp, kDefaultKp, kDefaultKi, kDefaultKd, QuickPID::Action::direct)
	, m_ssr(static_cast<gpio_num_t>(kGPIOPin_SSR))
	, m_brewSwitchGPIO(static_cast<gpio_num_t>(kGPIOPin_BrewSwitch))
	, m_steamSwitchGPIO(static_cast<gpio_num_t>(kGPIOPin_SteamSwitch))
	, m_terms(kDefaultKp, kDefaultKi, kDefaultKd)
{
	m_pid.SetSampleTimeUs(10000);
	m_pid.SetOutputLimits(0, 1024);
	m_pid.SetMode(QuickPID::Control::automatic);

	gpio_config_t gpioConfig = {};

	// Init Switches
	gpioConfig.mode = GPIO_MODE_INPUT;
	gpioConfig.pull_up_en = GPIO_PULLUP_ENABLE;

	gpioConfig.pin_bit_mask = 1ULL << kGPIOPin_BrewSwitch;
	gpio_config(&gpioConfig);

	gpioConfig.pin_bit_mask = 1ULL << kGPIOPin_SteamSwitch;
	gpio_config(&gpioConfig);
}

void BoilerController::updateCurrentTemp(float temp)
{
	if (m_currentTemp == temp)
		return;

	m_currentTemp = temp;
}

void BoilerController::setBrewTargetTemp(float temp)
{
	if (temp < kBrewTempMax)
		m_brewTemp = temp;
}

void BoilerController::setSteamTargetTemp(float temp)
{
	if (temp < kShutdownTemp)
		m_steamTemp = temp;
}

void BoilerController::setPIDTerms(PIDTerms terms)
{
	if (terms == m_terms)
		return;

	m_terms = terms;

	auto [Kp, Ki, Kd] = terms;

	m_pid.SetTunings(Kp, Ki, Kd);
}

void BoilerController::tick()
{
	if (m_inhibit)
		return;

	if (m_currentTemp > kShutdownTemp)
	{
		shutdown();
		return;
	}

	//TODO: Trigger pump
	//if (gpio_get_level(m_brewSwitchGPIO) == 0)

	if (gpio_get_level(m_steamSwitchGPIO) == 0)
		m_targetTemp = m_steamTemp;
	else
		m_targetTemp = m_brewTemp;

	m_pid.Compute();

	m_ssr.update(static_cast<int>(m_outputPower));
}

void BoilerController::shutdown()
{
	m_inhibit = true;
	m_targetTemp = 0.0f;
	m_ssr.update(0);
}
