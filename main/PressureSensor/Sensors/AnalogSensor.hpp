#pragma once

#include "PressureSensor.hpp"

#include "ADS1115.hpp"

#include <memory>

class AnalogSensor : public PressureSensor
{
public:
	explicit AnalogSensor();

	float GetPressure() override;

private:
	ADS1115		m_ADC;
};
