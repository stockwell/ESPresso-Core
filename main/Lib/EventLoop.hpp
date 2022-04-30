#pragma once

#include "esp_event.h"

class EventLoop
{
public:
	EventLoop(const char* id);
	virtual ~EventLoop() = default;

	void						eventPost(int32_t eventId, size_t dataSize = 0, void* data = nullptr);

protected:
	virtual void				eventHandler(int32_t eventId, void* data) = 0;
	esp_event_loop_handle_t		m_eventLoopHandle;

private:
	static void					eventAdapter(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
	esp_event_base_t			m_eventBase;
};
