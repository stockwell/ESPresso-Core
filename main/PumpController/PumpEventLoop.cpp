#include "PumpEventLoop.hpp"

namespace
{
	enum Events : uint8_t
	{
		TickTimerElapsed,

		UpdateCurrentPressure,
		SetTargetPressure,
		SetBrewPressure,

		GetCurrentPressure,
		GetTargetPressure,
		GetBrewPressure,

		GetPumpState,

		GetPIDTerms,
		SetPIDTerms,

		StartPump,
		StopPump,

		Shutdown,
	};
}

void PumpEventLoop::setPressure(PressureTypes type, float pressure)
{
	eventPost(Events::UpdateCurrentPressure + type, sizeof pressure, &pressure);
}

float PumpEventLoop::getPressure(PressureTypes type)
{
	return getFromEventLoop(Events::GetCurrentPressure + type, new std::promise<float>());
}

PumpController::PumpState PumpEventLoop::getState()
{
	return getFromEventLoop(Events::GetPumpState, new std::promise<PumpController::PumpState>());
}

PumpController::PIDTerms PumpEventLoop::getPIDTerms()
{
	return getFromEventLoop(Events::GetPIDTerms, new std::promise<PumpController::PIDTerms>());
}

void PumpEventLoop::setPIDTerms(PumpController::PIDTerms terms)
{
	eventPost(Events::SetPIDTerms, sizeof terms, &terms);

}

void PumpEventLoop::startPump()
{
	eventPost(Events::StartPump);
}

void PumpEventLoop::stopPump()
{
	eventPost(Events::StopPump);
}

void PumpEventLoop::shutdown()
{
	eventPost(Events::Shutdown);
}

PumpEventLoop::PumpEventLoop()
	: EventLoop("PumpEvent")
{
	m_timer = std::make_unique<Timer>(10, [this]() {
		eventPost(Events::TickTimerElapsed);
	});

	m_timer->start();
}

void PumpEventLoop::eventHandler(int32_t eventId, void* data)
{
	switch (eventId)
	{
	case Events::TickTimerElapsed:
		m_controller.tick();
		break;

	case Events::UpdateCurrentPressure:
		m_controller.updateCurrentPressure(*static_cast<float*>(data));
		break;

	case Events::SetBrewPressure:
		m_controller.setBrewPressure(*static_cast<float*>(data));
		break;

	case Events::GetCurrentPressure:
		EventLoopHelpers::setResponse(m_controller.getCurrentPressure(), data);
		break;

	case Events::GetTargetPressure:
		EventLoopHelpers::setResponse(m_controller.getTargetPressure(), data);
		break;

	case Events::GetBrewPressure:
		EventLoopHelpers::setResponse(m_controller.getBrewPressure(), data);
		break;

	case Events::GetPumpState:
		EventLoopHelpers::setResponse(m_controller.getState(), data);
		break;

	case Events::GetPIDTerms:
		EventLoopHelpers::setResponse(m_controller.getPIDTerms(), data);
		break;

	case Events::SetPIDTerms:
		m_controller.setPIDTerms(*static_cast<PumpController::PIDTerms*>(data));
		break;

	case Events::StartPump:
		m_controller.start();
		break;

	case Events::StopPump:
		m_controller.stop();
		break;

	case Events::Shutdown:
		m_controller.shutdown();
		m_timer->stop();
		break;

	default:
		printf("Unhandled event! %d\n", eventId);
		break;
	}
}
