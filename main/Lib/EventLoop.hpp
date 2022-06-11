#pragma once

#include "esp_event.h"

#include <future>

namespace EventLoopHelpers
{
	template<typename T>
	void setResponse(T val, void* data)
	{
		auto* promise = static_cast<std::promise<T>**>(data);
		(*promise)->set_value(val);
		delete *promise;
	}
}

class EventLoop
{
public:
	EventLoop(const char* id, size_t stackSize = 4096);
	virtual ~EventLoop() = default;

	void						eventPost(int32_t eventId, size_t dataSize = 0, void* data = nullptr);

protected:
	template<typename T>
	T getFromEventLoop(uint8_t event, std::promise<T>* promise)
	{
		auto future = promise->get_future();

		eventPost(event, sizeof(void*), &promise);

		future.wait();

		return future.get();
	}

	virtual void				eventHandler(int32_t eventId, void* data) = 0;
	esp_event_loop_handle_t		m_eventLoopHandle;

private:
	static void					eventAdapter(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
	esp_event_base_t			m_eventBase;
};
