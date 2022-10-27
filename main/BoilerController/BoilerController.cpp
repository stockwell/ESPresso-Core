#include "BoilerController.hpp"

namespace
{
	constexpr float kDefaultKp = 100.0f;
	constexpr float kDefaultKi = 50.0f;
	constexpr float kDefaultKd = 115.0f;

	constexpr auto kGPIOPin_SSR = GPIO_NUM_39;
	constexpr auto kGPIOPin_BrewSwitch = GPIO_NUM_42;
	constexpr auto kGPIOPin_BrewSwitch2 = GPIO_NUM_40;
	constexpr auto kGPIOPin_SteamSwitch = GPIO_NUM_41;

	constexpr auto kBrewTempMax = 100.0f;
	constexpr auto kShutdownTempUpper = 160.0f;
	constexpr auto kShutdownTempLower = -10.0f;
}

BoilerController::BoilerController(PumpEventLoop* pumpAPI)
	: m_pid(&m_currentTemp, &m_outputPower, &m_targetTemp, kDefaultKp, kDefaultKi, kDefaultKd, QuickPID::Action::direct)
	, m_ssr(kGPIOPin_SSR)
	, m_brewSwitchGPIO(kGPIOPin_BrewSwitch)
	, m_steamSwitchGPIO(kGPIOPin_SteamSwitch)
	, m_terms(kDefaultKp, kDefaultKi, kDefaultKd)
	, m_pumpAPI(pumpAPI)
{
	m_pid.SetOutputLimits(0, 1024);
	m_pid.SetSampleTimeUs(50 * 1000);
	m_pid.SetMode(QuickPID::Control::automatic);
	m_pid.SetProportionalMode(QuickPID::pMode::pOnErrorMeas);

	gpio_config_t gpioConfig = {};

	// Init Switches
	gpioConfig.mode = GPIO_MODE_OUTPUT;
	gpioConfig.pin_bit_mask = 1ULL << kGPIOPin_BrewSwitch2;
	gpio_config(&gpioConfig);
	gpio_set_level(kGPIOPin_BrewSwitch2, 1);

	gpioConfig.pull_up_en = GPIO_PULLUP_ENABLE;
	gpioConfig.mode = GPIO_MODE_INPUT;
	gpioConfig.pin_bit_mask = 1ULL << kGPIOPin_BrewSwitch;
	gpio_config(&gpioConfig);

	gpioConfig.pin_bit_mask = 1ULL << kGPIOPin_SteamSwitch;
	gpio_config(&gpioConfig);
}

void BoilerController::updateCurrentTemp(float temp)
{
	m_averageTemp(temp);

	m_currentTemp = m_averageTemp.get();
}

void BoilerController::setBrewTargetTemp(float temp)
{
	if (temp < kBrewTempMax)
		m_brewTemp = temp;
}

void BoilerController::setSteamTargetTemp(float temp)
{
	if (temp < kShutdownTempUpper)
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

	if (m_currentTemp < kShutdownTempLower || m_currentTemp > kShutdownTempUpper)
	{
		shutdown();
		return;
	}

	if (gpio_get_level(m_brewSwitchGPIO) == 0)
	{
		m_state = BoilerState::Brewing;
		m_pumpAPI->startPump();
	}
	else
	{
		m_state = BoilerState::Ready;
		m_pumpAPI->stopPump();
	}


	if (gpio_get_level(m_steamSwitchGPIO) == 0)
		m_targetTemp = m_steamTemp;
	else
		m_targetTemp = m_brewTemp;

	if (m_pid.Ready())
	{
		m_pid.Compute();
		m_ssr.update(static_cast<int>(m_outputPower));
	}


}

void BoilerController::shutdown()
{
	m_inhibit = true;
	m_targetTemp = 0.0f;
	m_ssr.update(0);
}
