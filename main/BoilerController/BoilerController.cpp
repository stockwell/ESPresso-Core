#include "BoilerController.hpp"

BoilerController::BoilerController()
{
    m_currentTemp = 0.0;
    m_targetTemp = 20.0;

    m_P = 500.0;
    m_I = 500;
    m_D = 1.0;

    m_pid = std::make_unique<QuickPID>(&m_currentTemp, &m_outputPower, &m_targetTemp, m_P, m_I, m_D, QuickPID::Action::direct);
    m_pid->SetSampleTimeUs(1000);
    m_pid->SetOutputLimits(0, 100);
    m_pid->SetMode(QuickPID::Control::automatic);
}

void BoilerController::registerBoilerTemperatureDelegate(BoilerTemperatureDelegate* delegate)
{
    if (m_delegates.find(delegate) != m_delegates.end())
        return;

    m_delegates.emplace(delegate);
}

void BoilerController::deregisterBoilerTemperatureDelegate(BoilerTemperatureDelegate* delegate)
{
    if (auto it = m_delegates.find(delegate); it != m_delegates.end())
        m_delegates.erase(it);
}

void BoilerController::setBoilerCurrentTemp(float temp)
{
    if (m_currentTemp == temp)
        return;

    m_currentTemp = temp;

    for (auto delegate : m_delegates)
        delegate->onBoilerCurrentTempChanged(temp);
}

void BoilerController::setBoilerTargetTemp(float temp)
{
    if (m_targetTemp == temp)
        return;

    m_targetTemp = temp;

    for (const auto delegate : m_delegates)
        delegate->onBoilerTargetTempChanged(temp);
}

void BoilerController::tick()
{
    m_pid->Compute();

    if (m_outputPower > 0)
        setBoilerCurrentTemp(m_currentTemp + 0.1);
    else
        setBoilerCurrentTemp(m_currentTemp - 0.1);
}
