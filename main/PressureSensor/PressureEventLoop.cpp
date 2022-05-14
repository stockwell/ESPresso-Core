#include "PressureEventLoop.hpp"

#include "Sensors/AnalogSensor.hpp"

namespace
{
	constexpr auto kPressurePollPeriodMs = 200;
	constexpr auto kGPIOPinGate = GPIO_NUM_12;
	constexpr auto kGPIOPinZeroCrossing = GPIO_NUM_14;

	enum Events
	{
		PressurePollTimerElapsed,
		SetTRIACDuty,
	};
}

void PressureEventLoop::setTRIACDuty(uint16_t value)
{
	eventPost(Events::SetTRIACDuty, sizeof(value), &value);
}

PressureEventLoop::PressureEventLoop()
	: EventLoop("PressureEvent")
{
	//m_sensor = std::make_unique<AnalogSensor>();

	//m_TRIAC = std::make_unique<TRIAC>(kGPIOPinGate, kGPIOPinZeroCrossing);

	m_timer = std::make_unique<Timer>(kPressurePollPeriodMs, [this]() {
		eventPost(Events::PressurePollTimerElapsed);
	});

	m_timer->start();
}

void PressureEventLoop::eventHandler(int32_t eventId, void* data)
{
	switch (eventId)
	{
	case Events::PressurePollTimerElapsed:
		// Update pressure in pump controller?
		//printf("Pressure: %.02f\n", m_sensor->GetPressure());
		break;

	case Events::SetTRIACDuty:
		//m_TRIAC->setDuty(*static_cast<uint16_t*>(data));
		printf("TRIAC duty set to %d\n", *static_cast<uint16_t*>(data));
		break;

	}
}
