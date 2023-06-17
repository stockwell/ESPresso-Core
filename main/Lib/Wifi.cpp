#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "mdns.h"

#include "Secrets.hpp"

#include "Wifi.hpp"

#include <string_view>

namespace
{
	constexpr std::string_view kSoftAP_ssid 	= "CoffeeDirect";
	constexpr std::string_view kSoftAP_password	= "pixel-archway-cajole";
	constexpr auto kSoftAP_channel				= 6;

	const char* TAG = "Wifi";
	static SemaphoreHandle_t s_semaphoreGetIP;
}

static void onWifiDisconnect(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	ESP_LOGI(TAG, "Wi-Fi disconnected, trying to reconnect...");
	esp_err_t err = esp_wifi_connect();

	if (err == ESP_ERR_WIFI_NOT_STARTED)
		return;

	ESP_ERROR_CHECK(err);
}

static void onWifiConnect(void* esp_netif, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	ESP_LOGI(TAG, "Wi-Fi connected!");
}

static void onGotIP(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	auto* event = static_cast<ip_event_got_ip_t*>(event_data);

	ESP_LOGI(TAG, "Got IPv4 event: Interface \"%s\" address: " IPSTR, esp_netif_get_desc(event->esp_netif), IP2STR(&event->ip_info.ip));
	xSemaphoreGive(s_semaphoreGetIP);
}

static void InitSoftAP()
{
	esp_netif_create_default_wifi_ap();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	wifi_config_t wifi_config = {};
	auto* ap = &wifi_config.ap;

	kSoftAP_ssid.copy((char*)ap->ssid, kSoftAP_ssid.size());
	kSoftAP_password.copy((char*)ap->password, kSoftAP_password.size());

	ap->channel 		= kSoftAP_channel;
	ap->max_connection	= 4;
	ap->authmode		= WIFI_AUTH_WPA_WPA2_PSK;

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT40));
	ESP_ERROR_CHECK(esp_wifi_start());
}

static void InitSTA()
{
	s_semaphoreGetIP = xSemaphoreCreateBinary();

	char *desc;
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();

	// Prefix the interface description with the module TAG
	// Warning: the interface desc is used in tests to capture actual connection details (IP, gw, mask)
	asprintf(&desc, "%s: %s", TAG, esp_netif_config.if_desc);
	esp_netif_config.if_desc = desc;
	esp_netif_config.route_prio = 128;
	esp_netif_t* netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);
	free(desc);

	esp_wifi_set_default_wifi_sta_handlers();

	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &onWifiDisconnect, nullptr));
	ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &onWifiConnect));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &onGotIP, nullptr));

#ifdef CONFIG_EXAMPLE_CONNECT_IPV6
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &on_wifi_connect, netif));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_GOT_IP6, &on_got_ipv6, NULL));
#endif

	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
	wifi_config_t wifi_config = {};
	auto* sta = &wifi_config.sta;

	Secrets::kSTA_ssid.copy((char*)sta->ssid, Secrets::kSTA_ssid.size());
	Secrets::kSTA_password.copy((char*)sta->password, Secrets::kSTA_password.size());

	sta->scan_method = WIFI_ALL_CHANNEL_SCAN;
	sta->sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
	sta->threshold.authmode = WIFI_AUTH_WEP;

	ESP_LOGI(TAG, "Connecting to %s...", sta->ssid);
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	esp_wifi_connect();

	xSemaphoreTake(s_semaphoreGetIP, portMAX_DELAY);

	esp_wifi_set_ps(WIFI_PS_NONE);
}

void Wifi::InitWifi(Wifi::WifiMode mode)
{
	switch(mode)
	{
		case WifiMode::SoftAP:
			InitSoftAP();
			break;

		case WifiMode::STA:
			InitSTA();
			break;
	}
}

void Wifi::InitMDNS()
{
	mdns_init();
	mdns_hostname_set("COFFEE");
	mdns_instance_name_set("COFFEE");

	mdns_txt_item_t serviceTxtData[] = {
		{"board", "esp32"},
	};

	ESP_ERROR_CHECK(mdns_service_add("COFFEE", "_http", "_tcp", 80, serviceTxtData,
		sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}
