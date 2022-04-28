#pragma once

class TemperatureSensor
{
public:
	explicit TemperatureSensor() = default;
	virtual ~TemperatureSensor() = default;

	virtual float GetTemperature() const = 0;
};
