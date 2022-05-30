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

		Shutdown,
	};
}

void BoilerEventLoop::setTemperature(TemperatureTypes type, float temperature)
{
	eventPost(Events::UpdateCurrentTemperature + type, sizeof(temperature), &temperature);
}

float BoilerEventLoop::getTemperature(TemperatureTypes type)
{
	auto prom = new std::promise<float>();
	std::future<float> fut = prom->get_future();

	eventPost(Events::GetCurrentTemperature + type, sizeof(void*), &prom);

	fut.wait();
	return fut.get();
}

void BoilerEventLoop::setPIDTerms(BoilerController::PIDTerms terms)
{
	eventPost(Events::SetPIDTerms, sizeof(terms), &terms);
}

void BoilerEventLoop::shutdown()
{
	eventPost(Events::Shutdown);
}

BoilerController::PIDTerms BoilerEventLoop::getPIDTerms()
{
	auto prom = new std::promise<BoilerController::PIDTerms>();
	std::future<BoilerController::PIDTerms> fut = prom->get_future();

	eventPost(Events::GetPIDTerms, sizeof(void*), &prom);

	fut.wait();
	return fut.get();
}

BoilerController::BoilerState BoilerEventLoop::getState()
{
	auto prom = new std::promise<BoilerController::BoilerState>();
	std::future<BoilerController::BoilerState> fut = prom->get_future();

	eventPost(Events::GetBoilerState, sizeof(void*), &prom);

	fut.wait();
	return fut.get();
}

BoilerEventLoop::BoilerEventLoop()
	: EventLoop("BoilerEvent")
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
		auto* prom = static_cast<std::promise<float>**>(data);
		auto val = m_controller.getCurrentTemp();

		(*prom)->set_value(val);
		delete *prom;

		break;
	}

	case Events::GetTargetTemperature:
	{
		auto* prom = static_cast<std::promise<float>**>(data);
		auto val = m_controller.getTargetTemp();

		(*prom)->set_value(val);
		delete *prom;

		break;
	}

	case Events::GetBrewTargetTemperature:
	{
		auto* prom = static_cast<std::promise<float>**>(data);
		auto val = m_controller.getBrewTargetTemp();

		(*prom)->set_value(val);
		delete *prom;

		break;
	}

	case Events::GetSteamTargetTemperature:
	{
		auto* prom = static_cast<std::promise<float>**>(data);
		auto val = m_controller.getSteamTargetTemp();

		(*prom)->set_value(val);
		delete *prom;

		break;
	}

	case Events::SetPIDTerms:
		m_controller.setPIDTerms(*static_cast<BoilerController::PIDTerms*>(data));
		break;

	case Events::GetPIDTerms:
	{
		auto* prom = static_cast<std::promise<BoilerController::PIDTerms>**>(data);
		(*prom)->set_value(m_controller.getPIDTerms());
		delete *prom;
		break;
	}

	case Events::GetBoilerState:
	{
		auto* prom = static_cast<std::promise<BoilerController::BoilerState>**>(data);
		(*prom)->set_value(m_controller.getState());
		delete *prom;
		break;
	}

	case Events::Shutdown:
		m_timer->stop();
		m_controller.shutdown();
		break;

	default:
		printf("Unhandled event! %d\n", eventId);
		break;
	}
}
