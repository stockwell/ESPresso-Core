#pragma once

#include "TemperatureSensor.hpp"

#include "Max31865.h"

class TemperatureSensorMAX31865 : public TemperatureSensor
{
public:
	explicit TemperatureSensorMAX31865();

	float GetTemperature() override;

private:
	Max31865	m_sensor;
};
