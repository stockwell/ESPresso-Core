#include "BoilerEventLoop.hpp"

#include <memory>
#include <future>

namespace
{
	enum Events
	{
		TickTimerElapsed,

		UpdateTemperature,
		SetTemperature,
		GetTemperature,

		SetPIDTerms,
		GetPIDTerms,
	};
}

void BoilerEventLoop::updateBoilerTemperature(float temperature)
{
	eventPost(Events::UpdateTemperature, sizeof(temperature), &temperature);
}

void BoilerEventLoop::setBoilerTemperature(float temperature)
{
	eventPost(Events::SetTemperature, sizeof(temperature), &temperature);
}

void BoilerEventLoop::setPIDTerms(BoilerController::PIDTerms terms)
{
	eventPost(Events::SetPIDTerms, sizeof(terms), &terms);
}

float BoilerEventLoop::getBoilerTemperature()
{
	auto prom = new std::promise<float>();
	std::future<float> fut = prom->get_future();

	eventPost(Events::GetTemperature, sizeof(void*), &prom);

	fut.wait();
	return fut.get();
}

BoilerController::PIDTerms BoilerEventLoop::getPIDTerms()
{
	auto prom = new std::promise<BoilerController::PIDTerms>();
	std::future<BoilerController::PIDTerms> fut = prom->get_future();

	eventPost(Events::GetPIDTerms, sizeof(void*), &prom);

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

	case Events::UpdateTemperature:
		m_controller.setBoilerCurrentTemp(*static_cast<float*>(data));
		break;

	case Events::SetTemperature:
		m_controller.setBoilerTargetTemp(*static_cast<float*>(data));
		break;

	case Events::GetTemperature:
	{
		auto* prom = static_cast<std::promise<float>**>(data);
		auto val = m_controller.getBoilerCurrentTemp();

		(*prom)->set_value(val);
		delete *prom;

		break;
	}

	case Events::SetPIDTerms:
	{
		m_controller.setPIDTerms(*static_cast<BoilerController::PIDTerms*>(data));
		break;
	}

	case Events::GetPIDTerms:
	{
		auto* prom = static_cast<std::promise<BoilerController::PIDTerms>**>(data);
		(*prom)->set_value(m_controller.getPIDTerms());
		delete *prom;
		break;
	}

	default:
		printf("Unhandled event! %d\n", eventId);
		break;
	}
}
