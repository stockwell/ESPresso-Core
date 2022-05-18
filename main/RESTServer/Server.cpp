/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <cstring>

#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"

#include "RESTServer/Server.hpp"

#include "UpdaterEventLoop.hpp"

namespace
{
	static constexpr auto		kScratchSize = 0x2800;
	struct ServerCtx
	{
		std::array<char, kScratchSize>		buffer;
		BoilerEventLoop*					boilerAPI;
		PressureEventLoop*					pressureAPI;
		std::unique_ptr<UpdaterEventLoop>	updaterEventLoop;
	};
}

static esp_err_t validate_post_request(httpd_req_t* req)
{
	int total_len = req->content_len;
	int cur_len = 0;
	auto& buf = ((ServerCtx*)(req->user_ctx))->buffer;
	if (total_len >= buf.size())
	{
		/* Respond with 500 Internal Server Error */
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
		return ESP_FAIL;
	}

	int received = 0;
	while (cur_len < total_len)
	{
		received = httpd_req_recv(req, buf.data() + cur_len, total_len);
		if (received <= 0)
		{
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

	auto* serverCtx = ((ServerCtx*)(req->user_ctx));

	cJSON* root = cJSON_Parse(serverCtx->buffer.data());
	if (! root)
	{
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
		return ESP_FAIL;
	}

	float Kp = cJSON_GetObjectItem(root, "Kp")->valuedouble;
	float Ki = cJSON_GetObjectItem(root, "Ki")->valuedouble;
	float Kd = cJSON_GetObjectItem(root, "Kd")->valuedouble;

	serverCtx->boilerAPI->setPIDTerms({Kp, Ki, Kd});

	cJSON_Delete(root);

	httpd_resp_sendstr(req, "");
	return ESP_OK;
}

static esp_err_t pid_terms_get_handler(httpd_req_t *req)
{
	auto* serverCtx = ((ServerCtx*)(req->user_ctx));

	auto [Kp, Ki, Kd] = serverCtx->boilerAPI->getPIDTerms();

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
	auto* serverCtx = ((ServerCtx*)(req->user_ctx));

	httpd_resp_set_type(req, "application/json");
	cJSON *root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "current", serverCtx->boilerAPI->getBoilerTemperature());

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

	auto* serverCtx = ((ServerCtx*)(req->user_ctx));

	cJSON* root = cJSON_Parse(serverCtx->buffer.data());
	if (! root)
	{
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
		return ESP_FAIL;
	}

	float target = cJSON_GetObjectItem(root, "target")->valuedouble;

	serverCtx->boilerAPI->setBoilerTemperature(target);

	cJSON_Delete(root);

	httpd_resp_sendstr(req, "");
	return ESP_OK;
}

static esp_err_t update_init_post_handler(httpd_req_t* req)
{
	if (auto err = validate_post_request(req); err != ESP_OK)
		return err;

	auto* serverCtx = ((ServerCtx*)(req->user_ctx));

	if (serverCtx->updaterEventLoop == nullptr)
		serverCtx->updaterEventLoop = std::make_unique<UpdaterEventLoop>();

	cJSON* root = cJSON_Parse(serverCtx->buffer.data());
	if (! root)
	{
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
		return ESP_FAIL;
	}

	auto* url = cJSON_GetObjectItem(root, "URL")->valuestring;
	auto* uuid = cJSON_GetObjectItem(root, "UUID")->valuestring;

	Updater::UpdateRequest request = {};
	strncpy(request.URL, url, sizeof(request.URL)-1);
	strncpy(request.UUID, uuid, sizeof(request.UUID)-1);

	if (! serverCtx->updaterEventLoop->initiateUpdate(request))
	{
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
		return ESP_FAIL;
	}

	serverCtx->boilerAPI->suspend();

	return ESP_OK;
}

static esp_err_t update_status_get_handler(httpd_req_t* req)
{
	auto* serverCtx = ((ServerCtx*)(req->user_ctx));
	auto status = serverCtx->updaterEventLoop->getUpdateStatus();

	httpd_resp_set_type(req, "application/json");
	cJSON *root = cJSON_CreateObject();

	cJSON_AddNumberToObject(root, "progress", status.progress);
	cJSON_AddStringToObject(root, "uuid", status.UUID.c_str());

	const char* temps = cJSON_Print(root);
	httpd_resp_sendstr(req, temps);

	free((void*)temps);
	cJSON_Delete(root);

	return ESP_OK;
}

static void registerURIHandler(httpd_handle_t server, const char* uri, http_method method, esp_err_t (*handler)(httpd_req_t *r), ServerCtx* ctx)
{
	httpd_uri_t http_uri =
	{
		.uri = uri,
		.method = method,
		.handler = handler,
		.user_ctx = ctx
	};

	httpd_register_uri_handler(server, &http_uri);
}

RESTServer::RESTServer(BoilerEventLoop* boiler, PressureEventLoop* pressure)
{
	auto* serverCtx = new ServerCtx;
	serverCtx->boilerAPI = boiler;
	serverCtx->pressureAPI = pressure;

	httpd_handle_t server = nullptr;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.uri_match_fn = httpd_uri_match_wildcard;

	ESP_LOGI(__FUNCTION__, "Starting HTTP Server");
	if (auto err = httpd_start(&server, &config); err != ESP_OK)
		throw std::runtime_error("Start server failed");

	registerURIHandler(server, "/api/v1/sys/info", HTTP_GET, sys_info_get_handler, serverCtx);
	registerURIHandler(server, "/api/v1/temp/raw", HTTP_GET, temperature_data_get_handler, serverCtx);
	registerURIHandler(server, "/api/v1/temp/raw", HTTP_POST, temperature_data_post_handler, serverCtx);
	registerURIHandler(server, "/api/v1/pid/terms", HTTP_GET, pid_terms_get_handler, serverCtx);
	registerURIHandler(server, "/api/v1/pid/terms", HTTP_POST, pid_terms_post_handler, serverCtx);

	registerURIHandler(server, "/api/v1/update/initiate", HTTP_POST, update_init_post_handler, serverCtx);
	registerURIHandler(server, "/api/v1/update/status", HTTP_GET, update_status_get_handler, serverCtx);
}
