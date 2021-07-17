#pragma once

#include "Engine/Helpers/Log.h"

//--- MACROS! 
#define LOG_CRITICAL(...)	Log::getInstance().Logger()->critical(__VA_ARGS__);
#define LOG_ERROR(...)		Log::getInstance().Logger()->error(__VA_ARGS__);
#define LOG_WARNING(...)	Log::getInstance().Logger()->warn(__VA_ARGS__);
#define LOG_INFO(...)		Log::getInstance().Logger()->info(__VA_ARGS__);
#define LOG_DEBUG(...)		Log::getInstance().Logger()->debug(__VA_ARGS__);

#define SAFE_DELETE(x)		if(x) { delete x; x = nullptr; }

#define M_PI				3.14159265358979323846   // pi
#define M_PI_OVER_TWO		1.57079632679489661923   // pi/2
#define M_PI_OVER_FOUR		0.785398163397448309616  // pi/4
#define ONE_OVER_PI			0.318309886183790671538  // 1/pi
#define TWO_OVER_PI			0.636619772367581343076  // 2/pi