#pragma once

class PressureSensor
{
public:
	explicit PressureSensor() = default;
	virtual ~PressureSensor() = default;

	virtual float GetPressure() = 0;
};
