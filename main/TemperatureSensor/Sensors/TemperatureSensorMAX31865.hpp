#pragma once

#include "TemperatureSensor.hpp"

#include "Max31865.h"

class TemperatureSensorMAX31865 : public TemperatureSensor
{
public:
	explicit TemperatureSensorMAX31865();
	virtual ~TemperatureSensorMAX31865() = default;

	float GetTemperature() const override;

private:
	Max31865	m_sensor;
};
