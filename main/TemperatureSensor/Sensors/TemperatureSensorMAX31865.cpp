#include "TemperatureSensorMAX31865.hpp"
#include "driver/gpio.h"

namespace
{
	constexpr auto kGPIOPinMISO = CONFIG_MAX31865_PIN_MISO;
	constexpr auto kGPIOPinMOSI = CONFIG_MAX31865_PIN_MOSI;
	constexpr auto kGPIOPinSCLK = CONFIG_MAX31865_PIN_SCLK;
	constexpr auto kGPIOPinCS   = CONFIG_MAX31865_PIN_CS;
	constexpr auto kGPIOPinDRDY = CONFIG_MAX31865_PIN_DRDY;

	constexpr auto kRTDMin = 0x0000;
	constexpr auto kRTDMax = 0x5000;
	constexpr auto kRTDRef = 430.0f;
	constexpr auto kRTDNom = 100.0f;
}

TemperatureSensorMAX31865::TemperatureSensorMAX31865()
	: m_sensor(kGPIOPinMISO, kGPIOPinMOSI, kGPIOPinSCLK, kGPIOPinCS)
{
	const max31865_config_t config =
	{
		.vbias			= true,
		.autoConversion	= true,
		.nWires			= Max31865NWires::Three,
		.faultDetection	= Max31865FaultDetection::NoAction,
		.filter			= Max31865Filter::Hz50,
	};

	ESP_ERROR_CHECK(m_sensor.begin(config));
	ESP_ERROR_CHECK(m_sensor.setRTDThresholds(kRTDMin, kRTDMax));
}

float TemperatureSensorMAX31865::GetTemperature()
{
	const max31865_rtd_config_t rtdConfig =
	{
		.ref			= kRTDRef,
		.nominal		= kRTDNom,
	};

	uint16_t rtd		= 0;
	Max31865Error fault	= Max31865Error::NoError;

	while (gpio_get_level(static_cast<gpio_num_t>(CONFIG_MAX31865_PIN_DRDY)) == 1)
		vTaskDelay(1);

	m_sensor.getRTD(&rtd, &fault);
	m_sensor.clearFault();

	return Max31865::RTDtoTemperature(rtd, rtdConfig);
}
