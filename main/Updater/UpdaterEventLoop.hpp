#pragma once

#include "Lib/EventLoop.hpp"
#include "Lib/Timer.hpp"

#include <memory>
#include <string>

class Updater;

class UpdaterEventLoop : public EventLoop
{
public:
	UpdaterEventLoop();
	~UpdaterEventLoop() = default;

	struct UpdateRequest
	{
		char URL[128];
		char UUID[64];
		size_t filesize;
	};

	struct UpdateStatus
	{
		std::string UUID;
		size_t progress;
		// status code - failed to download, timeout etc
	};

	bool 			initiateUpdate(UpdateRequest& request);
	UpdateStatus	getUpdateStatus();

protected:
	void			eventHandler(int32_t eventId, void* data) override;

private:
	std::mutex					m_lock;
	std::unique_ptr<Updater>	m_updater;
};
