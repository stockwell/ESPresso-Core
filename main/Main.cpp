#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "RESTServer/Server.hpp"

#include "BoilerEventLoop.hpp"
#include "PressureEventLoop.hpp"
#include "TemperatureEventLoop.hpp"
#include "Wifi.hpp"

static void mainTask(void* pvParameter)
{
	(void)pvParameter;

	auto ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	auto boilerEventLoop = std::make_unique<BoilerEventLoop>();
	auto temperatureEventLoop = std::make_unique<TemperatureEventLoop>(boilerEventLoop.get());
	auto pressureEventLoop = std::make_unique<PressureEventLoop>();
	auto restServer = std::make_unique<RESTServer>(boilerEventLoop.get(), pressureEventLoop.get());

	boilerEventLoop->setTemperature(BoilerEventLoop::BrewTargetTemp, 93.1f);
	boilerEventLoop->setTemperature(BoilerEventLoop::SteamTargetTemp, 140.0f);

	Wifi::InitWifi(Wifi::WifiMode::STA);
	Wifi::InitMDNS();

	while (1)
		vTaskDelay(portMAX_DELAY);
}

extern "C" void app_main()
{
	xTaskCreatePinnedToCore(mainTask, "main", 4096, nullptr, 0, nullptr, 1);
}
