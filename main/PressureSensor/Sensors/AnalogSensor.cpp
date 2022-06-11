#include "AnalogSensor.hpp"

#include "driver/i2c.h"

namespace
{
	// TODO: SDKConfig
	constexpr auto kGPIONumSDA	= 35;
	constexpr auto kGPIONumSCL	= 36;
	constexpr auto kI2CFreq		= 400000;
	constexpr auto kI2CNum		= I2C_NUM_0;
}

AnalogSensor::AnalogSensor()
	: m_ADC(kI2CNum)
{
	i2c_config_t i2c_cfg = {
		.mode			= I2C_MODE_MASTER,
		.sda_io_num		= kGPIONumSDA,
		.scl_io_num		= kGPIONumSCL,
		.sda_pullup_en	= GPIO_PULLUP_DISABLE,
		.scl_pullup_en	= GPIO_PULLUP_DISABLE,
	};

	i2c_cfg.master.clk_speed = kI2CFreq;

	i2c_param_config(kI2CNum, &i2c_cfg);
	i2c_driver_install(kI2CNum, I2C_MODE_MASTER, 0, 0, 0);
}

float AnalogSensor::GetPressure()
{
	const auto voltage = m_ADC.getVoltage();
	constexpr float kVoltageZeroPoint = 0.5;

	return (voltage - kVoltageZeroPoint) * 3.0f;
}
