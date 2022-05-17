#pragma once

#include "esp_ota_ops.h"

#include <string>

class Updater
{
public:
	struct UpdateRequest
	{
		std::string URL;
		std::string UUID;
	};

	bool initiate(const UpdateRequest& request);

};
