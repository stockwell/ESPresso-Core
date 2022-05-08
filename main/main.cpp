#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "esp_system.h"
#include "mdns.h"
#include "nvs_flash.h"

#include "RESTServer/Server.hpp"

#include "BoilerEventLoop.hpp"
#include "PressureEventLoop.hpp"
#include "TemperatureEventLoop.hpp"
#include "Wifi.hpp"

/*********************
 *      DEFINES
 *********************/
#define TAG "EspressoUI"

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void mainTask(void* pvParameter);

/**********************
 *   APPLICATION MAIN
 **********************/
extern "C" void app_main()
{
	xTaskCreatePinnedToCore(mainTask, "main", 4096, NULL, 0, NULL, 1);
}

static void initialise_mdns(void)
{
	mdns_init();
	mdns_hostname_set("COFFEE");
	mdns_instance_name_set("COFFEE");

	mdns_txt_item_t serviceTxtData[] = {
		{"board", "esp32"},
		{"path", "/"}
	};

	ESP_ERROR_CHECK(mdns_service_add("COFFEE", "_http", "_tcp", 80, serviceTxtData,
		sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}

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
	initialise_mdns();

	Wifi::InitSoftAP();

	auto boilerEventLoop = std::make_unique<BoilerEventLoop>();
	boilerEventLoop->setBoilerTemperature(93.1f);

	auto temperatureEventLoop = std::make_unique<TemperatureEventLoop>(boilerEventLoop.get());

	auto pressureEventLoop = std::make_unique<PressureEventLoop>();

	ESP_ERROR_CHECK(start_rest_server(boilerEventLoop.get()));

	while (1)
	{
		vTaskDelay(portMAX_DELAY);
	}

	vTaskDelete(NULL);
}
