#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"

#include <string_view>

#include "Wifi.hpp"

namespace
{
	constexpr std::string_view kSoftAP_ssid 	= "CoffeeDirect";
	constexpr std::string_view kSoftAP_password	= "mattsarah";
}


void Wifi::InitSoftAP()
{
	esp_netif_create_default_wifi_ap();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	wifi_config_t wifi_config = {};
	auto* ap = &wifi_config.ap;

	kSoftAP_ssid.copy((char*)ap->ssid, kSoftAP_ssid.size());
	kSoftAP_password.copy((char*)ap->password, kSoftAP_password.size());

	ap->channel 		= 6;
	ap->max_connection	= 4;
	ap->authmode		= WIFI_AUTH_WPA_WPA2_PSK;

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT40));
	ESP_ERROR_CHECK(esp_wifi_start());
}
