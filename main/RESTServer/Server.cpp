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

#include "Updater.hpp"
#include "UpdaterEventLoop.hpp"

namespace
{
	static constexpr auto		kScratchSize = 0x2800;
	struct ServerCtx
	{
		std::array<char, kScratchSize>		buffer;
		BoilerEventLoop*					boilerAPI;
		PressureEventLoop*					pressureAPI;
		PumpEventLoop*						pumpAPI;
		TemperatureEventLoop*				temperatureAPI;
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

	if (auto boilerPID = cJSON_GetObjectItem(root, "BoilerPID"); boilerPID != nullptr)
	{
		if (cJSON_GetArraySize(boilerPID) == 3)
		{
			float Kp = cJSON_GetArrayItem(boilerPID, 0)->valuedouble;
			float Ki = cJSON_GetArrayItem(boilerPID, 1)->valuedouble;
			float Kd = cJSON_GetArrayItem(boilerPID, 2)->valuedouble;

			serverCtx->boilerAPI->setPIDTerms({Kp, Ki, Kd});
		}
	}

	if (auto pumpPID = cJSON_GetObjectItem(root, "PumpPID"); pumpPID != nullptr)
	{
		if (cJSON_GetArraySize(pumpPID) == 3)
		{
			float Kp = cJSON_GetArrayItem(pumpPID, 0)->valuedouble;
			float Ki = cJSON_GetArrayItem(pumpPID, 1)->valuedouble;
			float Kd = cJSON_GetArrayItem(pumpPID, 2)->valuedouble;

			serverCtx->pumpAPI->setPIDTerms({Kp, Ki, Kd});
		}
	}

	cJSON_Delete(root);

	httpd_resp_sendstr(req, "");
	return ESP_OK;
}

static esp_err_t pid_terms_get_handler(httpd_req_t *req)
{
	auto* serverCtx = ((ServerCtx*)(req->user_ctx));

	auto [BoilerKp, BoilerKi, BoilerKd] = serverCtx->boilerAPI->getPIDTerms();
	auto [PumpKp, PumpKi, PumpKd] = serverCtx->pumpAPI->getPIDTerms();

	httpd_resp_set_type(req, "application/json");
	cJSON *root = cJSON_CreateObject();

	auto boilerPID = cJSON_AddArrayToObject(root, "BoilerPID");
	cJSON_AddItemToArray(boilerPID, cJSON_CreateNumber(BoilerKp));
	cJSON_AddItemToArray(boilerPID, cJSON_CreateNumber(BoilerKi));
	cJSON_AddItemToArray(boilerPID, cJSON_CreateNumber(BoilerKd));

	auto pumpPID = cJSON_AddArrayToObject(root, "PumpPID");
	cJSON_AddItemToArray(pumpPID, cJSON_CreateNumber(PumpKp));
	cJSON_AddItemToArray(pumpPID, cJSON_CreateNumber(PumpKi));
	cJSON_AddItemToArray(pumpPID, cJSON_CreateNumber(PumpKd));

	const char* temps = cJSON_Print(root);
	httpd_resp_sendstr(req, temps);

	free((void*)temps);
	cJSON_Delete(root);
	return ESP_OK;
}

static esp_err_t temperature_data_get_handler(httpd_req_t *req)
{
	auto* boilerAPI = ((ServerCtx*)(req->user_ctx))->boilerAPI;

	httpd_resp_set_type(req, "application/json");
	cJSON *root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "current", boilerAPI->getTemperature(BoilerEventLoop::CurrentBoilerTemp));
	cJSON_AddNumberToObject(root, "target",  boilerAPI->getTemperature(BoilerEventLoop::CurrentTargetTemp));
	cJSON_AddNumberToObject(root, "brew",    boilerAPI->getTemperature(BoilerEventLoop::BrewTargetTemp));
	cJSON_AddNumberToObject(root, "steam",   boilerAPI->getTemperature(BoilerEventLoop::SteamTargetTemp));

	cJSON_AddNumberToObject(root, "state",  static_cast<int>(boilerAPI->getState()));

	const char* temps = cJSON_Print(root);
	httpd_resp_sendstr(req, temps);

	free((void*)temps);
	cJSON_Delete(root);
	return ESP_OK;
}

static esp_err_t pressure_data_get_handler(httpd_req_t *req)
{
	auto* pumpAPI = ((ServerCtx*)(req->user_ctx))->pumpAPI;

	httpd_resp_set_type(req, "application/json");
	cJSON *root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "current", pumpAPI->getPressure(PumpEventLoop::CurrentPressure));
	cJSON_AddNumberToObject(root, "target", pumpAPI->getPressure(PumpEventLoop::TargetPressure));
	cJSON_AddNumberToObject(root, "brew", pumpAPI->getPressure(PumpEventLoop::BrewTargetPressure));
	cJSON_AddNumberToObject(root, "manual-duty", pumpAPI->getManualDuty());
	cJSON_AddBoolToObject(root, "manual-mode", pumpAPI->getManualMode());

	cJSON_AddNumberToObject(root, "state",  static_cast<int>(pumpAPI->getState()));

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
	cJSON_AddNumberToObject(root, "min_free_heap", esp_get_minimum_free_heap_size());

	const char* info = cJSON_Print(root);
	httpd_resp_sendstr(req, info);

	free((void*)info);
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

	if (cJSON_HasObjectItem(root, "brewTarget"))
		serverCtx->boilerAPI->setTemperature(BoilerEventLoop::BrewTargetTemp, cJSON_GetObjectItem(root, "brewTarget")->valuedouble);

	if (cJSON_HasObjectItem(root, "steamTarget"))
		serverCtx->boilerAPI->setTemperature(BoilerEventLoop::SteamTargetTemp, cJSON_GetObjectItem(root, "steamTarget")->valuedouble);

	cJSON_Delete(root);

	httpd_resp_sendstr(req, "");
	return ESP_OK;
}

static esp_err_t pressure_data_post_handler(httpd_req_t* req)
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

	if (cJSON_HasObjectItem(root, "brewTarget"))
		serverCtx->pumpAPI->setPressure(PumpEventLoop::BrewTargetPressure, cJSON_GetObjectItem(root, "brewTarget")->valuedouble);

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
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, nullptr);
		return ESP_FAIL;
	}

	const auto* url = cJSON_GetObjectItem(root, "URL");
	const auto* uuid = cJSON_GetObjectItem(root, "UUID");
	const auto* size = cJSON_GetObjectItem(root, "filesize");

	if (! url || ! uuid || ! size)
	{
		cJSON_Delete(root);
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, nullptr);
		return ESP_FAIL;
	}

	UpdaterEventLoop::UpdateRequest request = {};
	strncpy(request.URL, url->valuestring, sizeof request.URL - 1);
	strncpy(request.UUID, uuid->valuestring, sizeof request.UUID - 1);
	request.filesize = size->valueint;

	if (! serverCtx->updaterEventLoop->initiateUpdate(request))
	{
		cJSON_Delete(root);
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, nullptr);
		return ESP_FAIL;
	}

	// TODO: Generic shutdown notification system
	serverCtx->boilerAPI->shutdown();
	serverCtx->pressureAPI->shutdown();
	serverCtx->pumpAPI->shutdown();
	serverCtx->temperatureAPI->shutdown();

	httpd_resp_sendstr(req, "Update Initiated");
	cJSON_Delete(root);

	return ESP_OK;
}

static esp_err_t update_status_get_handler(httpd_req_t* req)
{
	auto* serverCtx = ((ServerCtx*)(req->user_ctx));

	UpdaterEventLoop::UpdateStatus status = {};

	if (serverCtx->updaterEventLoop != nullptr)
		status = serverCtx->updaterEventLoop->getUpdateStatus();

	httpd_resp_set_type(req, "application/json");
	cJSON *root = cJSON_CreateObject();

	cJSON_AddNumberToObject(root, "progress", status.progress);
	cJSON_AddStringToObject(root, "uuid", status.UUID.c_str());

	const char* statusJSON = cJSON_Print(root);
	httpd_resp_sendstr(req, statusJSON);

	free((void*)statusJSON);
	cJSON_Delete(root);

	return ESP_OK;
}

static esp_err_t clear_inhibit_post_handler(httpd_req_t* req)
{
	if (auto err = validate_post_request(req); err != ESP_OK)
		return err;

	auto* serverCtx = ((ServerCtx*)(req->user_ctx));

	serverCtx->boilerAPI->clearInhibit();

	httpd_resp_sendstr(req, "");
	return ESP_OK;
}

static esp_err_t manual_control_post_handler(httpd_req_t* req)
{
	if (auto err = validate_post_request(req); err != ESP_OK)
		return err;

	auto* serverCtx = ((ServerCtx*)(req->user_ctx));

	cJSON* root = cJSON_Parse(serverCtx->buffer.data());

	if (! root)
	{
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, nullptr);
		return ESP_FAIL;
	}

	const auto* duty = cJSON_GetObjectItem(root, "Duty");
	const auto* enabled = cJSON_GetObjectItem(root, "ManualControl");

	if (duty == nullptr || enabled == nullptr)
	{
		cJSON_Delete(root);
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, nullptr);
		return ESP_FAIL;
	}

	serverCtx->pumpAPI->setManualDuty(duty->valuedouble);
	serverCtx->pumpAPI->setManualMode(enabled->type == cJSON_True);

	cJSON_Delete(root);

	httpd_resp_sendstr(req, "");
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

RESTServer::RESTServer(BoilerEventLoop* boiler, PressureEventLoop* pressure, PumpEventLoop* pump, TemperatureEventLoop* temp)
{
	auto* serverCtx = new ServerCtx;
	serverCtx->boilerAPI = boiler;
	serverCtx->pressureAPI = pressure;
	serverCtx->pumpAPI = pump;
	serverCtx->temperatureAPI = temp;

	httpd_handle_t server = nullptr;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.uri_match_fn = httpd_uri_match_wildcard;
	config.max_uri_handlers = 16;

	ESP_LOGI(__FUNCTION__, "Starting HTTP Server");
	if (auto err = httpd_start(&server, &config); err != ESP_OK)
		throw std::runtime_error("Start server failed");

	registerURIHandler(server, "/api/v1/sys/info", HTTP_GET, sys_info_get_handler, serverCtx);
	registerURIHandler(server, "/api/v1/temp/raw", HTTP_GET, temperature_data_get_handler, serverCtx);
	registerURIHandler(server, "/api/v1/temp/raw", HTTP_POST, temperature_data_post_handler, serverCtx);
	registerURIHandler(server, "/api/v1/pid/terms", HTTP_GET, pid_terms_get_handler, serverCtx);
	registerURIHandler(server, "/api/v1/pid/terms", HTTP_POST, pid_terms_post_handler, serverCtx);

	registerURIHandler(server, "/api/v1/boiler/clear-inhibit", HTTP_POST, clear_inhibit_post_handler, serverCtx);
	registerURIHandler(server, "/api/v1/pump/manual-control", HTTP_POST, manual_control_post_handler, serverCtx);

	registerURIHandler(server, "/api/v1/pressure/raw", HTTP_GET, pressure_data_get_handler, serverCtx);
	registerURIHandler(server, "/api/v1/pressure/raw", HTTP_POST, pressure_data_post_handler, serverCtx);

	registerURIHandler(server, "/api/v1/update/initiate", HTTP_POST, update_init_post_handler, serverCtx);
	registerURIHandler(server, "/api/v1/update/status", HTTP_GET, update_status_get_handler, serverCtx);
}
