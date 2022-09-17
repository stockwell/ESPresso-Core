#pragma once

#include "UpdaterEventLoop.hpp"

#include "esp_ota_ops.h"
#include "esp_http_client.h"

#include <string>

class Updater
{
public:
	Updater() = default;
	~Updater() = default;

	bool initiate(const UpdaterEventLoop::UpdateRequest& request);
	UpdaterEventLoop::UpdateStatus getStatus() const { return m_status; }

private:
	void run(const UpdaterEventLoop::UpdateRequest& request);
	void cleanup();

private:
	static constexpr auto kBufferSize = 1024;
	char buffer[kBufferSize + 1];

	UpdaterEventLoop::UpdateStatus		m_status;
	esp_http_client_handle_t			m_client;
};
