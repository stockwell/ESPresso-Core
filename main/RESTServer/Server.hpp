#pragma once

#include "BoilerEventLoop.hpp"
#include "PressureEventLoop.hpp"

class RESTServer
{
public:
	explicit RESTServer(BoilerEventLoop* boiler, PressureEventLoop* pressure);
};
