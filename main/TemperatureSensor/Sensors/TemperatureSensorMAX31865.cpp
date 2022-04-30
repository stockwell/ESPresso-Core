#include "TemperatureSensorMAX31865.hpp"

namespace
{
	// TODO: Get from SDKConfig
	constexpr auto kGPIOPinMISO = 13;
	constexpr auto kGPIOPinMOSI = 14;
	constexpr auto kGPIOPinSCLK = 15;
	constexpr auto kGPIOPinCS   = 16;

	constexpr auto kRTDMin = 0x2000;
	constexpr auto kRTDMax = 0x2500;
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
		.faultDetection	= Max31865FaultDetection::AutoDelay,
		.filter			= Max31865Filter::Hz50,
	};

	ESP_ERROR_CHECK(m_sensor.begin(config));
	ESP_ERROR_CHECK(m_sensor.setRTDThresholds(0x2000, 0x2500));
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

	m_sensor.getRTD(&rtd, &fault);

	return Max31865::RTDtoTemperature(rtd, rtdConfig);
}
