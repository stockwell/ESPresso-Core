#pragma once

class TemperatureSensor
{
public:
	explicit TemperatureSensor() = default;
	virtual ~TemperatureSensor() = default;

	virtual float GetTemperature() = 0;
};
