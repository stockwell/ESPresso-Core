#include "EventLoop.hpp"

#include <stdexcept>

EventLoop::EventLoop(const char* eventBase)
	: m_eventBase(eventBase)
{
	esp_event_loop_args_t eventTaskArgs =
	{
		.queue_size = 10,
		.task_name = eventBase,
		.task_priority = 1,
		.task_stack_size = 2048,
		.task_core_id = tskNO_AFFINITY
	};

	if (auto ret = esp_event_loop_create(&eventTaskArgs, &m_eventLoopHandle); ret != ESP_OK)
	{
		printf("Fatal: Failed to create event loop!\n");
		abort();
	}

	esp_event_handler_register_with(m_eventLoopHandle, m_eventBase, -1, eventAdapter, this);
}

void EventLoop::eventPost(int32_t eventId, size_t dataSize, void* data)
{
	if (auto ret = esp_event_post_to(m_eventLoopHandle, m_eventBase, eventId, data, dataSize, 0); ret != ESP_OK)
	{
		printf("Fatal: Failed to post event to %s\n", m_eventBase);
		abort();
	}
}

void EventLoop::eventAdapter(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	auto eventLoop = static_cast<EventLoop*>(event_handler_arg);

	eventLoop->eventHandler(event_id, event_data);
}
