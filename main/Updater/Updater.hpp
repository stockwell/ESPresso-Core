#pragma once

#include "esp_ota_ops.h"

#include <string>

class Updater
{
public:
	struct UpdateRequest
	{
		char URL[128];
		char UUID[64];
	};

	bool initiate(const UpdateRequest& request);

};
