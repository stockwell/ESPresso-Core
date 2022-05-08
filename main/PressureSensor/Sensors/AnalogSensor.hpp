#pragma once

#include "PressureSensor.hpp"

#include <memory>

class ADS1115;

class AnalogSensor : public PressureSensor
{
public:
	explicit AnalogSensor();

	float GetPressure();

private:
	std::unique_ptr<ADS1115>	m_ADC;
};
