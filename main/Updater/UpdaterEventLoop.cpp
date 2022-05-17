#include "UpdaterEventLoop.hpp"

#include <future>

namespace
{
	enum Events
	{
		UpdateTimeout,

		Initiate,
	};
}

bool UpdaterEventLoop::initiateUpdate(const UpdateRequest& request)
{
	if (request.URL.empty() || request.UUID.empty())
		return false;

	eventPost(Events::Initiate, sizeof(request), (void*)&request);

	return false;
}

UpdaterEventLoop::UpdateStatus UpdaterEventLoop::getUpdateStatus()
{
	UpdateStatus status = {};

	return status;
}

UpdaterEventLoop::UpdaterEventLoop()
	: EventLoop("UpdaterEventLoop")
{

}

void UpdaterEventLoop::eventHandler(int32_t eventId, void* data)
{
	switch (eventId)
	{
		case Events::Initiate:
		{
			auto req = *static_cast<UpdateRequest*>(data);

			printf("Update request initiated with UUID %s @ %s\n", req.UUID.c_str(), req.URL.c_str());
			break;
		}
	}
}
