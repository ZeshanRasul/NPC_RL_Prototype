#pragma once

#include <cstdio>

class Logger {
public:
	template <typename... Args>
	static void Log(unsigned int logLevel, Args ... args) {
		if (logLevel <= m_logLevel) {
			std::printf(args ...);
		}
	}

	static void SetLogLevel(unsigned int inLogLevel) {
		inLogLevel <= 9 ? m_logLevel = inLogLevel : m_logLevel = 9;
	}

private:
	static unsigned int m_logLevel;
};