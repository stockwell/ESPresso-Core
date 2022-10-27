#include "BoilerEventLoop.hpp"

#include <memory>
#include <future>

namespace
{
	enum Events : uint8_t
	{
		TickTimerElapsed,

		GetCurrentTemperature,
		GetTargetTemperature,
		GetBrewTargetTemperature,
		GetSteamTargetTemperature,

		UpdateCurrentTemperature,
		SetTargetTemperature,
		SetBrewTargetTemperature,
		SetSteamTargetTemperature,

		SetPIDTerms,
		GetPIDTerms,

		GetBoilerState,

		ClearInhibit,
		Shutdown,
	};
}

void BoilerEventLoop::setTemperature(TemperatureTypes type, float temperature)
{
	eventPost(Events::UpdateCurrentTemperature + type, sizeof(temperature), &temperature);
}

float BoilerEventLoop::getTemperature(TemperatureTypes type)
{
	return getFromEventLoop(Events::GetCurrentTemperature + type, new std::promise<float>());
}

void BoilerEventLoop::setPIDTerms(BoilerController::PIDTerms terms)
{
	eventPost(Events::SetPIDTerms, sizeof(terms), &terms);
}

void BoilerEventLoop::clearInhibit()
{
	eventPost(Events::ClearInhibit);
}

void BoilerEventLoop::shutdown()
{
	eventPost(Events::Shutdown);
}

BoilerController::PIDTerms BoilerEventLoop::getPIDTerms()
{
	return getFromEventLoop(Events::GetPIDTerms, new std::promise<BoilerController::PIDTerms>());
}

BoilerController::BoilerState BoilerEventLoop::getState()
{
	return getFromEventLoop(Events::GetBoilerState, new std::promise<BoilerController::BoilerState>());
}

BoilerEventLoop::BoilerEventLoop(PumpEventLoop* pumpAPI)
	: EventLoop("BoilerEvent")
	, m_controller(pumpAPI)
{
	m_timer = std::make_unique<Timer>(10, [this]() {
		eventPost(Events::TickTimerElapsed);
	});

	m_timer->start();
}

void BoilerEventLoop::eventHandler(int32_t eventId, void* data)
{
	switch (eventId)
	{
	case Events::TickTimerElapsed:
		m_controller.tick();
		break;

	case Events::UpdateCurrentTemperature:
		m_controller.updateCurrentTemp(*static_cast<float*>(data));
		break;

	case Events::SetBrewTargetTemperature:
		m_controller.setBrewTargetTemp(*static_cast<float*>(data));
		break;

	case Events::SetSteamTargetTemperature:
		m_controller.setSteamTargetTemp(*static_cast<float*>(data));
		break;

	case Events::GetCurrentTemperature:
	{
		EventLoopHelpers::setResponse(m_controller.getCurrentTemp(), data);
		break;
	}

	case Events::GetTargetTemperature:
	{
		EventLoopHelpers::setResponse(m_controller.getTargetTemp(), data);
		break;
	}

	case Events::GetBrewTargetTemperature:
	{
		EventLoopHelpers::setResponse(m_controller.getBrewTargetTemp(), data);
		break;
	}

	case Events::GetSteamTargetTemperature:
	{
		EventLoopHelpers::setResponse(m_controller.getSteamTargetTemp(), data);
		break;
	}

	case Events::SetPIDTerms:
		m_controller.setPIDTerms(*static_cast<BoilerController::PIDTerms*>(data));
		break;

	case Events::GetPIDTerms:
	{
		EventLoopHelpers::setResponse(m_controller.getPIDTerms(), data);
		break;
	}

	case Events::GetBoilerState:
	{
		EventLoopHelpers::setResponse(m_controller.getState(), data);
		break;
	}

	case Events::ClearInhibit:
		m_controller.clearInhibit();
		break;

	case Events::Shutdown:
		m_timer->stop();
		m_controller.shutdown();
		break;

	default:
		printf("Unhandled event! %d\n", eventId);
		break;
	}
}
