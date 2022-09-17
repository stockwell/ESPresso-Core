#pragma once

#include "BoilerEventLoop.hpp"
#include "PressureEventLoop.hpp"
#include "TemperatureEventLoop.hpp"

class RESTServer
{
public:
	explicit RESTServer(BoilerEventLoop* boiler, PressureEventLoop* pressure, PumpEventLoop* pump, TemperatureEventLoop* temp);
};
