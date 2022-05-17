#include "Updater.hpp"
#include "esp_log.h"
#include "esp_http_client.h"

static void task_fatal_error()
{
	abort();
}

static void http_cleanup(esp_http_client_handle_t client)
{
	esp_http_client_close(client);
	esp_http_client_cleanup(client);
}

constexpr auto kBufferSize = 1024;
static char ota_write_data[kBufferSize + 1] = { 0 };

static void ota_example_task(std::string_view URL)
{
	const char* TAG = "update";

	esp_err_t err;
	/* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
	esp_ota_handle_t update_handle = 0 ;
	const esp_partition_t *update_partition = nullptr;

	ESP_LOGI(TAG, "Starting OTA example");

	const esp_partition_t *configured = esp_ota_get_boot_partition();
	const esp_partition_t *running = esp_ota_get_running_partition();

	if (configured != running) {
		ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
			configured->address, running->address);
		ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
	}
	ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
		running->type, running->subtype, running->address);

	esp_http_client_config_t config = {
		.url = URL.data(),
		.cert_pem = nullptr,
		.timeout_ms = 25,
		.skip_cert_common_name_check = true,
		.keep_alive_enable = true,
	};

	esp_http_client_handle_t client = esp_http_client_init(&config);
	if (client == nullptr)
	{
		ESP_LOGE(TAG, "Failed to initialise HTTP connection");
		task_fatal_error();
	}
	err = esp_http_client_open(client, 0);
	if (err != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
		esp_http_client_cleanup(client);
		task_fatal_error();
	}
	esp_http_client_fetch_headers(client);

	update_partition = esp_ota_get_next_update_partition(nullptr);
	assert(update_partition != nullptr);
	ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
		update_partition->subtype, update_partition->address);

	int binary_file_length = 0;
	/*deal with all receive packet*/
	bool image_header_was_checked = false;
	while (1)
	{
		int data_read = esp_http_client_read(client, ota_write_data, kBufferSize);
		if (data_read < 0)
		{
			ESP_LOGE(TAG, "Error: SSL data read error");
			http_cleanup(client);
			task_fatal_error();
		}
		else if (data_read > 0)
		{
			if (! image_header_was_checked)
			{
				esp_app_desc_t new_app_info;
				if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t))
				{
					// check current version with downloading
					memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
					ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

					esp_app_desc_t running_app_info;
					if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK)
						ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);

					const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
					esp_app_desc_t invalid_app_info;

					if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK)
						ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);

					// check current version with last invalid partition
					if (last_invalid_app != nullptr)
					{
						if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0)
						{
							ESP_LOGW(TAG, "New version is the same as invalid version.");
							ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
							ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
							http_cleanup(client);
						}
					}

					image_header_was_checked = true;

					err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
					if (err != ESP_OK)
					{
						ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
						http_cleanup(client);
						esp_ota_abort(update_handle);
						task_fatal_error();
					}
					ESP_LOGI(TAG, "esp_ota_begin succeeded");
				}
				else
				{
					ESP_LOGE(TAG, "received package is not fit len");
					http_cleanup(client);
					esp_ota_abort(update_handle);
					task_fatal_error();
				}
			}
			err = esp_ota_write( update_handle, (const void *)ota_write_data, data_read);

			if (err != ESP_OK)
			{
				http_cleanup(client);
				esp_ota_abort(update_handle);
				task_fatal_error();
			}
			binary_file_length += data_read;
			ESP_LOGD(TAG, "Written image length %d", binary_file_length);
		}
		else if (data_read == 0)
		{
			/*
			 * As esp_http_client_read never returns negative error code, we rely on
			 * `errno` to check for underlying transport connectivity closure if any
			 */
			if (errno == ECONNRESET || errno == ENOTCONN)
			{
				ESP_LOGE(TAG, "Connection closed, errno = %d", errno);
				break;
			}

			if (esp_http_client_is_complete_data_received(client))
			{
				ESP_LOGI(TAG, "Connection closed");
				break;
			}
		}
	}
	ESP_LOGI(TAG, "Total Write binary data length: %d", binary_file_length);
	if (! esp_http_client_is_complete_data_received(client))
	{
		ESP_LOGE(TAG, "Error in receiving complete file");
		http_cleanup(client);
		esp_ota_abort(update_handle);
		task_fatal_error();
	}

	err = esp_ota_end(update_handle);
	if (err != ESP_OK)
	{
		if (err == ESP_ERR_OTA_VALIDATE_FAILED)
			ESP_LOGE(TAG, "Image validation failed, image is corrupted");
		else
			ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));

		http_cleanup(client);
		task_fatal_error();
	}

	err = esp_ota_set_boot_partition(update_partition);
	if (err != ESP_OK)
	{
		ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
		http_cleanup(client);
		task_fatal_error();
	}
	ESP_LOGI(TAG, "Prepare to restart system!");
	esp_restart();
}

bool Updater::initiate(const Updater::UpdateRequest& request)
{
	printf("Update request initiated with UUID %s @ %s\n", request.UUID.c_str(), request.URL.c_str());
	ota_example_task(request.URL);

	return true;
}
