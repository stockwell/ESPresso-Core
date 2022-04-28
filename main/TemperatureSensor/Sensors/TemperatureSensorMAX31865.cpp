#include "TemperatureSensorMAX31865.hpp"

namespace
{
	// TODO: Get from SDKConfig
	constexpr auto kGPIOPinMISO = 13;
	constexpr auto kGPIOPinMOSI = 14;
	constexpr auto kGPIOPinSCLK = 15;
	constexpr auto kGPIOPinCS   = 16;
}

TemperatureSensorMAX31865::TemperatureSensorMAX31865()
	: m_sensor(kGPIOPinMISO, kGPIOPinMOSI, kGPIOPinSCLK, kGPIOPinCS)
{

}

float TemperatureSensorMAX31865::GetTemperature() const
{
	return rand();
}
