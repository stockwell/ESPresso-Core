#pragma once

#include "Lib/EventLoop.hpp"
#include "Lib/Timer.hpp"

#include <memory>
#include <string>

class UpdaterEventLoop : public EventLoop
{
public:
	UpdaterEventLoop();

	struct UpdateStatus
	{
		float progress;
		std::string UUID;
		// status code - failed to download, timeout etc
	};

	struct UpdateRequest
	{
		std::string URL;
		std::string UUID;
	};

	bool 			initiateUpdate(const UpdateRequest& request);
	UpdateStatus	getUpdateStatus();

protected:
	void			eventHandler(int32_t eventId, void* data) override;

private:
	std::unique_ptr<Timer>	m_timer;

	std::string				UUID;
};
