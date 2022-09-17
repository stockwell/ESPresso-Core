#include "UpdaterEventLoop.hpp"

#include "Updater.hpp"

namespace
{
	enum Events
	{
		Initiate,
	};
}

bool UpdaterEventLoop::initiateUpdate(UpdateRequest& request)
{
	eventPost(Events::Initiate, sizeof request, &request);
	return true;
}

UpdaterEventLoop::UpdateStatus UpdaterEventLoop::getUpdateStatus()
{
	UpdateStatus status;
	{
		std::scoped_lock lock(m_lock);
		status = m_updater->getStatus();
	}

	return status;
}

UpdaterEventLoop::UpdaterEventLoop()
	: EventLoop("UpdaterEventLoop")
{
	m_updater = std::make_unique<Updater>();
}

void UpdaterEventLoop::eventHandler(int32_t eventId, void* data)
{
	switch (eventId)
	{
		case Events::Initiate:
		{
			const auto request = *static_cast<UpdateRequest*>(data);
			m_updater->initiate(request);
			break;
		}
	}
}
