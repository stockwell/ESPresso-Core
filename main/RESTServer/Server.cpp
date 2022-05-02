/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"

#include "RESTServer/Server.hpp"

static const char *REST_TAG = "esp-rest";
#define REST_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                 \
    {                                                                                  \
        if (!(a))                                                                      \
        {                                                                              \
            ESP_LOGE(REST_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                             \
        }                                                                              \
    } while (0)

#define SCRATCH_BUFSIZE (10240)

typedef struct rest_server_context {
	char scratch[SCRATCH_BUFSIZE];
	BoilerEventLoop*	boilerAPI;
} rest_server_context_t;

static esp_err_t validate_post_request(httpd_req_t* req)
{
	int total_len = req->content_len;
	int cur_len = 0;
	char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
	int received = 0;
	if (total_len >= SCRATCH_BUFSIZE) {
		/* Respond with 500 Internal Server Error */
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
		return ESP_FAIL;
	}
	while (cur_len < total_len) {
		received = httpd_req_recv(req, buf + cur_len, total_len);
		if (received <= 0) {
			/* Respond with 500 Internal Server Error */
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
			return ESP_FAIL;
		}
		cur_len += received;
	}
	buf[total_len] = '\0';

	return ESP_OK;
}

static esp_err_t pid_terms_post_handler(httpd_req_t* req)
{
	if (auto err = validate_post_request(req); err != ESP_OK)
		return err;

	char* buf = ((rest_server_context_t *)(req->user_ctx))->scratch;

	cJSON* root = cJSON_Parse(buf);
	if (! root)
	{
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
		return ESP_FAIL;
	}

	float Kp = cJSON_GetObjectItem(root, "Kp")->valuedouble;
	float Ki = cJSON_GetObjectItem(root, "Ki")->valuedouble;
	float Kd = cJSON_GetObjectItem(root, "Kd")->valuedouble;

	BoilerEventLoop* boilerAPI = ((rest_server_context_t *)(req->user_ctx))->boilerAPI;
	boilerAPI->setPIDTerms({Kp, Ki, Kd});

	cJSON_Delete(root);

	httpd_resp_sendstr(req, "");
	return ESP_OK;
}

static esp_err_t pid_terms_get_handler(httpd_req_t *req)
{
	BoilerEventLoop* boilerAPI = ((rest_server_context_t *)(req->user_ctx))->boilerAPI;

	auto [Kp, Ki, Kd] = boilerAPI->getPIDTerms();

	httpd_resp_set_type(req, "application/json");
	cJSON *root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "Kp", Kp);
	cJSON_AddNumberToObject(root, "Ki", Ki);
	cJSON_AddNumberToObject(root, "Kd", Kd);

	const char* temps = cJSON_Print(root);
	httpd_resp_sendstr(req, temps);
	free((void*)temps);
	cJSON_Delete(root);
	return ESP_OK;
}

static esp_err_t temperature_data_get_handler(httpd_req_t *req)
{
	BoilerEventLoop* boilerAPI = ((rest_server_context_t *)(req->user_ctx))->boilerAPI;

	httpd_resp_set_type(req, "application/json");
	cJSON *root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "current", boilerAPI->getBoilerTemperature());

	const char* temps = cJSON_Print(root);
	httpd_resp_sendstr(req, temps);
	free((void*)temps);
	cJSON_Delete(root);
	return ESP_OK;
}

static esp_err_t sys_info_get_handler(httpd_req_t *req)
{
	httpd_resp_set_type(req, "application/json");
	cJSON *root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "free_heap", esp_get_free_heap_size());

	const char* temps = cJSON_Print(root);
	httpd_resp_sendstr(req, temps);
	free((void*)temps);
	cJSON_Delete(root);
	return ESP_OK;
}

static esp_err_t temperature_data_post_handler(httpd_req_t* req)
{
	if (auto err = validate_post_request(req); err != ESP_OK)
		return err;

	char* buf = ((rest_server_context_t *)(req->user_ctx))->scratch;

	cJSON* root = cJSON_Parse(buf);
	if (! root)
	{
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
		return ESP_FAIL;
	}

	float target = cJSON_GetObjectItem(root, "target")->valuedouble;

	BoilerEventLoop* boilerAPI = ((rest_server_context_t *)(req->user_ctx))->boilerAPI;
	boilerAPI->setBoilerTemperature(target);

	cJSON_Delete(root);

	httpd_resp_sendstr(req, "");
	return ESP_OK;
}

esp_err_t start_rest_server(BoilerEventLoop* boiler)
{
	auto* rest_context = (rest_server_context_t*)(calloc(1, sizeof(rest_server_context_t)));

	rest_context->boilerAPI = boiler;

	httpd_handle_t server = nullptr;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.uri_match_fn = httpd_uri_match_wildcard;

	/* URI handler for fetching temperature data */
	httpd_uri_t temperature_data_get_uri =
	{
		.uri = "/api/v1/temp/raw",
		.method = HTTP_GET,
		.handler = temperature_data_get_handler,
		.user_ctx = rest_context
	};

	httpd_uri_t temperature_data_post_uri =
	{
		 .uri = "/api/v1/temp/raw",
		 .method = HTTP_POST,
		 .handler = temperature_data_post_handler,
		 .user_ctx = rest_context
	};

	httpd_uri_t pid_terms_post_uri =
	{
		.uri = "/api/v1/pid/terms",
		.method = HTTP_POST,
		.handler = pid_terms_post_handler,
		.user_ctx = rest_context
	};

	httpd_uri_t pid_terms_get_uri =
	{
		.uri = "/api/v1/pid/terms",
		.method = HTTP_GET,
		.handler = pid_terms_get_handler,
		.user_ctx = rest_context
	};

	httpd_uri_t sys_info_get_uri =
	{
		 .uri = "/api/v1/sys/info",
		 .method = HTTP_GET,
		 .handler = sys_info_get_handler,
		 .user_ctx = rest_context
	};

	REST_CHECK(rest_context, "No memory for rest context", err);

	ESP_LOGI(REST_TAG, "Starting HTTP Server");
	REST_CHECK(httpd_start(&server, &config) == ESP_OK, "Start server failed", err_start);

	httpd_register_uri_handler(server, &temperature_data_get_uri);
	httpd_register_uri_handler(server, &temperature_data_post_uri);
	httpd_register_uri_handler(server, &pid_terms_post_uri);
	httpd_register_uri_handler(server, &pid_terms_get_uri);
	httpd_register_uri_handler(server, &sys_info_get_uri);

	return ESP_OK;

err_start:
	free(rest_context);

err:
	return ESP_FAIL;
}
