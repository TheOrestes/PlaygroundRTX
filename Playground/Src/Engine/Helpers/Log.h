#pragma once

#include "spdlog/spdlog.h"

class Log
{
public:
	static Log& getInstance()
	{
		static Log instance;
		return instance;
	}

	~Log();

	inline spdlog::logger* Logger() { return m_pLogger; }

private:
	Log();

	Log(const Log&);						// prevent copies
	void operator=(const Log&);				// prevent assignments

	spdlog::logger* m_pLogger;
};

