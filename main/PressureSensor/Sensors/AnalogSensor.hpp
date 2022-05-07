#pragma once

#include "PressureSensor.hpp"

class AnalogSensor : public PressureSensor
{
public:


	float GetPressure() override { return 0.0f; };
};
