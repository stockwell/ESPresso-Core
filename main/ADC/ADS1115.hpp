#pragma once

#include <vector>

#include "driver/i2c.h"
#include "driver/gpio.h"

class ADS1115
{
public:

	enum Mux
	{
		MUX_0_1 = 0,
		MUX_0_3,
		MUX_1_3,
		MUX_2_3,
		MUX_0_GND,
		MUX_1_GND,
		MUX_2_GND,
		MUX_3_GND,
	};

	enum Resolution
	{ // full-scale resolution options
		FSR_6_144 = 0,
		FSR_4_096,
		FSR_2_048,
		FSR_1_024,
		FSR_0_512,
		FSR_0_256,
	};

	enum SPS
	{
		SPS_8 = 0,
		SPS_16,
		SPS_32,
		SPS_64,
		SPS_128,
		SPS_250,
		SPS_475,
		SPS_860
	};

	enum Mode
	{
		CONTINUOUS = 0,
		SINGLE
	};

	typedef union { // configuration register
		struct {
			uint16_t COMP_QUE:2;  // bits 0..  1  Comparator queue and disable
			uint16_t COMP_LAT:1;  // bit  2       Latching Comparator
			uint16_t COMP_POL:1;  // bit  3       Comparator Polarity
			uint16_t COMP_MODE:1; // bit  4       Comparator Mode
			uint16_t DR:3;        // bits 5..  7  Data rate
			uint16_t MODE:1;      // bit  8       Device operating mode
			uint16_t PGA:3;       // bits 9..  11 Programmable gain amplifier configuration
			uint16_t MUX:3;       // bits 12.. 14 Input multiplexer configuration
			uint16_t OS:1;        // bit  15      Operational status or single-shot conversion start
		} bit;
		uint16_t reg;
	} ADS1115_CONFIG_REGISTER_Type;

	explicit ADS1115(i2c_port_t i2c_port, uint8_t address = 0x48);

	void setMux(ADS1115::Mux mux) { m_config.bit.MUX = mux; m_configChanged = true; }
	void setPGA(ADS1115::Resolution resolution) { m_config.bit.PGA = resolution; m_configChanged = true; }
	void setMode(ADS1115::Mode mode) { m_config.bit.MODE = mode; m_configChanged = true; }
	void setSPS(ADS1115::SPS sps) { m_config.bit.DR = sps; m_configChanged = true; }
	void setMaxTicks(TickType_t maxTicks) { m_maxTicks = maxTicks; };
	void setRdyGPIOPin(gpio_num_t gpio);

	int16_t getRaw();
	float getVoltage();

private:
	esp_err_t writeRegister(uint8_t reg, uint16_t data);
	esp_err_t readRegister(uint8_t reg, uint8_t* data, size_t length);

private:
	int					m_address = -1;
	uint8_t 			m_lastReg = -1;
	TickType_t			m_maxTicks = 0;
	bool 				m_configChanged = false;

	gpio_num_t			m_rdyPin = GPIO_NUM_NC;

	i2c_port_t			m_i2c;
	SemaphoreHandle_t	m_rdySemaphore;
	ADS1115_CONFIG_REGISTER_Type m_config;
};
