#pragma once

#include "Updater.hpp"

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


	bool 			initiateUpdate(const Updater::UpdateRequest& request);
	UpdateStatus	getUpdateStatus();

protected:
	void			eventHandler(int32_t eventId, void* data) override;

private:
	std::unique_ptr<Timer>	m_timer;
	Updater					m_updater;
};
