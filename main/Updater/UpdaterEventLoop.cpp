#include "UpdaterEventLoop.hpp"

#include "Updater.hpp"

#include <future>

namespace
{
	enum Events
	{
		UpdateTimeout,

		Initiate,
	};
}

bool UpdaterEventLoop::initiateUpdate(Updater::UpdateRequest& request)
{
	eventPost(Events::Initiate, sizeof(request), &request);
	return true;
}

UpdaterEventLoop::UpdateStatus UpdaterEventLoop::getUpdateStatus()
{
	UpdateStatus status = {};

	return status;
}

UpdaterEventLoop::UpdaterEventLoop()
	: EventLoop("UpdaterEventLoop", 8192)
{

}

void UpdaterEventLoop::eventHandler(int32_t eventId, void* data)
{
	switch (eventId)
	{
		case Events::Initiate:
		{
			const auto request = *static_cast<Updater::UpdateRequest*>(data);

			m_updater.initiate(request);
			break;
		}
	}
}
