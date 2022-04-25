#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "mdns.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "RESTServer/Server.hpp"

#include "BoilerEventLoop.hpp"

/*********************
 *      DEFINES
 *********************/
#define TAG "EspressoUI"

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void guiTask(void* pvParameter);

/**********************
 *   APPLICATION MAIN
 **********************/
extern "C" void app_main()
{
	/* If you want to use a task to create the graphic, you NEED to create a Pinned task
	 * Otherwise there can be problem such as memory corruption and so on.
	 * NOTE: When not using Wi-Fi nor Bluetooth you can pin the guiTask to core 0 */
	xTaskCreatePinnedToCore(guiTask, "main", 4096, NULL, 0, NULL, 1);
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

static void guiTask(void* pvParameter)
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

	ESP_ERROR_CHECK(example_connect());

	auto boilerEventLoop = std::make_unique<BoilerEventLoop>();
	boilerEventLoop->setBoilerTemperature(93.1f);

	ESP_ERROR_CHECK(start_rest_server(boilerEventLoop.get()));

	while (1)
	{
		/* Delay 1 tick (assumes FreeRTOS tick is 10ms */
		vTaskDelay(pdMS_TO_TICKS(100));
	}

	vTaskDelete(NULL);
}
