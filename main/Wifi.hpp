#pragma once

namespace Wifi
{
	enum WifiMode
	{
		STA,
		SoftAP,
	};

	void InitWifi(WifiMode mode);
	void InitMDNS();
}
