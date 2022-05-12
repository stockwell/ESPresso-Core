#include "ADS1115.hpp"

#include <array>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

namespace
{
	enum RegisterAddr
	{
		CONVERSION = 0,
		CONFIG,
		LO_THRESH,
		HI_THRESH,
	};
}

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
	auto sem = static_cast<SemaphoreHandle_t>(arg);
	xSemaphoreGive(sem);
}

ADS1115::ADS1115(i2c_port_t i2c_port, uint8_t address)
	: m_address(address)
	, m_maxTicks(10 / portTICK_PERIOD_MS)
	, m_i2c(i2c_port)
{
	m_config.bit.OS			= 1; // always start conversion
	m_config.bit.MUX		= Mux::MUX_0_GND;
	m_config.bit.PGA		= Resolution::FSR_4_096;
	m_config.bit.MODE		= Mode::SINGLE;
	m_config.bit.DR			= SPS::SPS_64;
	m_config.bit.COMP_MODE	= 0;
	m_config.bit.COMP_POL	= 0;
	m_config.bit.COMP_LAT	= 0;
	m_config.bit.COMP_QUE	= 0b11;

	m_configChanged = true;
}

esp_err_t ADS1115::writeRegister(uint8_t reg, uint16_t data)
{
	i2c_cmd_handle_t cmd;
	esp_err_t ret;
	uint8_t out[2];
	out[0] = data >> 8;
	out[1] = data & 0xFF;

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (m_address << 1) | I2C_MASTER_WRITE, 1);
	i2c_master_write_byte(cmd, reg, true);
	i2c_master_write(cmd, out, 2, true);
	i2c_master_stop(cmd);

	ret = i2c_master_cmd_begin(m_i2c, cmd, m_maxTicks);
	i2c_cmd_link_delete(cmd);

	m_lastReg = reg;
	return ret;
}

esp_err_t ADS1115::readRegister(uint8_t reg, uint8_t* data, size_t length)
{
	i2c_cmd_handle_t cmd;
	esp_err_t ret;

	if (m_lastReg != reg)
	{
		cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd,(m_address << 1) | I2C_MASTER_WRITE, true);
		i2c_master_write_byte(cmd,reg, true);
		i2c_master_stop(cmd);
		i2c_master_cmd_begin(m_i2c, cmd, m_maxTicks);
		i2c_cmd_link_delete(cmd);
		m_lastReg = reg;
	}

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd,(m_address << 1) | I2C_MASTER_READ, true);
	i2c_master_read(cmd, data, length, I2C_MASTER_NACK);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(m_i2c, cmd, m_maxTicks);
	i2c_cmd_link_delete(cmd);
	return ret;
}


void ADS1115::setRdyGPIOPin(gpio_num_t gpio)
{
	gpio_config_t io_conf;
	io_conf.intr_type		= GPIO_INTR_NEGEDGE;
	io_conf.pin_bit_mask	= 1 << gpio;
	io_conf.mode			= GPIO_MODE_INPUT;
	io_conf.pull_up_en		= GPIO_PULLUP_ENABLE;
	io_conf.pull_down_en	= GPIO_PULLDOWN_DISABLE;
	gpio_config(&io_conf);

	m_rdySemaphore = xSemaphoreCreateBinary();
	gpio_install_isr_service(0);

	m_rdyPin				= gpio;
	m_config.bit.COMP_QUE	= 0b00; // assert after one conversion
	m_configChanged			= true;

	if (auto err = writeRegister(LO_THRESH, 0); err != 0) // set lo threshold to minimum
		ESP_LOGE(__FUNCTION__,"could not set low threshold: %s",esp_err_to_name(err));

	if (auto err = writeRegister(HI_THRESH, 0xFFFF); err != 0) // set lo threshold to minimum
		ESP_LOGE(__FUNCTION__,"could not set low threshold: %s",esp_err_to_name(err));
}

int16_t ADS1115::getRaw()
{
	static constexpr std::array sps = {8, 16, 32, 64, 128, 250, 475, 860};

	if (m_rdyPin != GPIO_NUM_NC)
	{
		m_rdySemaphore = xSemaphoreCreateBinary();
		gpio_isr_handler_add(m_rdyPin, gpio_isr_handler, (void*)m_rdySemaphore);
	}

	// see if we need to send configuration data
	if ((m_config.bit.MODE == SINGLE) || (m_configChanged))
	{
		if (auto err = writeRegister(CONFIG, m_config.reg); err != 0)
		{
			ESP_LOGE(__FUNCTION__, "could not write to device: %s", esp_err_to_name(err));
			if (m_rdyPin != GPIO_NUM_NC)
			{
				gpio_isr_handler_remove(m_rdyPin);
				vSemaphoreDelete(m_rdySemaphore);
			}
			return 0;
		}
		m_configChanged = false;
	}

	if (m_rdyPin != GPIO_NUM_NC)
	{
		xSemaphoreTake(m_rdySemaphore, portMAX_DELAY);
		gpio_isr_handler_remove(m_rdyPin);
	}
	else
	{
		// wait for 1 ms longer than the sampling rate, plus a little bit for rounding
		vTaskDelay((((1000 / sps[m_config.bit.DR]) + 1) / portTICK_PERIOD_MS) + 1);
	}

	uint8_t data[2];
	if (auto err = readRegister(CONVERSION, data, sizeof data); err != 0)
	{
		ESP_LOGE(__FUNCTION__, "could not read from device: %s", esp_err_to_name(err));
		return 0;
	}

	return (uint16_t)data[0] << 8 | data[1];
}

float ADS1115::getVoltage()
{
	static constexpr std::array fsr = {6.144f, 4.096f, 2.048f, 1.024f, 0.512f, 0.256f};
	constexpr int16_t bits = (1L << 15) - 1;
	int16_t raw = getRaw();

	return (float)raw * fsr[m_config.bit.PGA] / (float)bits;
}
