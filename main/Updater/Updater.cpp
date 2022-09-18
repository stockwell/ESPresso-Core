#include "Updater.hpp"

#include "esp_http_client.h"

bool Updater::initiate(const UpdaterEventLoop::UpdateRequest& request)
{
	printf("Update request initiated with UUID %s @ %s\n", request.UUID, request.URL);
	m_status.UUID = request.UUID;
	m_status.progress = 0;

	run(request);

	return true;
}

void Updater::cleanup()
{
	esp_http_client_cleanup(m_client);

	m_status.progress = -1;
	m_status.UUID = "";
}

void Updater::run(const UpdaterEventLoop::UpdateRequest& request)
{
	esp_err_t err;
	esp_ota_handle_t update_handle = 0;


	esp_http_client_config_t config = {};
	config.url = request.URL;
	config.cert_pem = nullptr;
	config.timeout_ms = 500;
	config.skip_cert_common_name_check = true;
	config.keep_alive_enable = true;

	esp_http_client_handle_t client = esp_http_client_init(&config);
	if (client == nullptr)
	{
		printf("Update: Failed to initialise HTTP connection\n");
		cleanup();
		return;
	}
	err = esp_http_client_open(client, 0);
	if (err != ESP_OK)
	{
		printf("Update: Failed to open HTTP connection: %s\n", esp_err_to_name(err));
		esp_http_client_cleanup(client);
		cleanup();
		return;
	}
	esp_http_client_fetch_headers(client);

	const esp_partition_t* runningPartition = esp_ota_get_running_partition();
	const esp_partition_t* updatePartition = esp_ota_get_next_update_partition(nullptr);
	assert(updatePartition != nullptr);

	size_t payloadLength = 0;
	bool headerChecked = false;
	while (true)
	{
		int data_read = esp_http_client_read(client, buffer, kBufferSize);
		if (data_read < 0)
		{
			printf("Update: data read error\n");
			esp_ota_abort(update_handle);
			cleanup();
			return;
		}
		else if (data_read > 0)
		{
			if (! headerChecked)
			{
				esp_app_desc_t new_app_info;
				if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t))
				{
					// check current version with downloading
					memcpy(&new_app_info, &buffer[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
					printf("Update: New firmware version: %s\n", new_app_info.version);

					esp_app_desc_t running_app_info;
					if (esp_ota_get_partition_description(runningPartition, &running_app_info) == ESP_OK)
						printf("Update: Running firmware version: %s\n", running_app_info.version);

					const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
					esp_app_desc_t invalid_app_info;

					if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK)
						printf("Update: Last invalid firmware version: %s\n", invalid_app_info.version);

					// check current version with last invalid partition
					if (last_invalid_app != nullptr)
					{
						if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0)
						{
							printf("New version is the same as invalid version.\n");
							printf("Previously, there was an attempt to launch the firmware with %s version, but it failed.\n", invalid_app_info.version);
							printf("The firmware has been rolled back to the previous version.\n");
							cleanup();
							return;
						}
					}

					headerChecked = true;

					err = esp_ota_begin(updatePartition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
					if (err != ESP_OK)
					{
						printf("Update: esp_ota_begin failed (%s)\n", esp_err_to_name(err));
						cleanup();
						esp_ota_abort(update_handle);
						return;
					}
					printf("Update: esp_ota_begin succeeded\n");
				}
				else
				{
					printf("Update: received package will not fit len\n");
					cleanup();
					esp_ota_abort(update_handle);
					return;
				}
			}

			err = esp_ota_write(update_handle, (const void *)buffer, data_read);

			if (err != ESP_OK)
			{
				cleanup();
				esp_ota_abort(update_handle);
				return;
			}
			payloadLength += data_read;
			m_status.progress = payloadLength;
		}
		else if (data_read == 0)
		{
			if (errno == ECONNRESET || errno == ENOTCONN)
			{
				printf("Update: Connection closed, errno = %d\n", errno);
				break;
			}

			if (esp_http_client_is_complete_data_received(client))
			{
				printf("Update: Connection closed\n");
				break;
			}
		}
	}
	printf("Update: Wrote %zu bytes\n", payloadLength);
	if (! esp_http_client_is_complete_data_received(client))
	{
		printf("Update: Error in receiving complete file\n");
		cleanup();
		esp_ota_abort(update_handle);
		return;
	}

	err = esp_ota_end(update_handle);
	if (err != ESP_OK)
	{
		if (err == ESP_ERR_OTA_VALIDATE_FAILED)
			printf("Update: Image validation failed, image is corrupted\n");
		else
			printf("Update: esp_ota_end failed (%s)!\n", esp_err_to_name(err));

		cleanup();
		return;
	}

	err = esp_ota_set_boot_partition(updatePartition);
	if (err != ESP_OK)
	{
		printf("Update: esp_ota_set_boot_partition failed (%s)!\n", esp_err_to_name(err));
		cleanup();
		return;
	}

	printf("Update: Complete, restarting..\n");

	// Delay for 1s to allow for server to get final status
	vTaskDelay(pdMS_TO_TICKS(1000));
	esp_restart();
}
