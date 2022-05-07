#include "SSRController.hpp"

#include "driver/ledc.h"

namespace
{
	constexpr auto kLEDC_Timer		= LEDC_TIMER_0;
	constexpr auto kLEDC_Mode		= LEDC_LOW_SPEED_MODE;
	constexpr auto kLEDC_Channel	= LEDC_CHANNEL_0;
	constexpr auto kLEDC_DutyRes	= LEDC_TIMER_10_BIT;
	constexpr auto kLEDC_Frequency	= 5;

}

SSRController::SSRController(gpio_num_t GPIONum /*TODO: AC Period*/)
{
	ledc_timer_config_t ledc_timer = {
		.speed_mode			= kLEDC_Mode,
		.duty_resolution	= kLEDC_DutyRes,
		.timer_num			= kLEDC_Timer,
		.freq_hz			= kLEDC_Frequency,
		.clk_cfg			= LEDC_AUTO_CLK
	};
	ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

	ledc_channel_config_t ledc_channel = {
		.gpio_num		= GPIONum,
		.speed_mode		= kLEDC_Mode,
		.channel		= kLEDC_Channel,
		.intr_type		= LEDC_INTR_DISABLE,
		.timer_sel		= kLEDC_Timer,
		.duty			= 0,
		.hpoint			= 0
	};
	ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}


void SSRController::update(uint16_t duty)
{
	ESP_ERROR_CHECK(ledc_set_duty(kLEDC_Mode, kLEDC_Channel, duty));
	ESP_ERROR_CHECK(ledc_update_duty(kLEDC_Mode, kLEDC_Channel));
}
