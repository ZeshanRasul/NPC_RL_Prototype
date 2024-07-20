#pragma once

#include <cstdio>

class Logger {
public:
    template <typename... Args>
    static void log(unsigned int logLevel, Args ... args) {
        if (logLevel <= mLogLevel) {
            std::printf(args ...);
        }
    }

    static void setLogLevel(unsigned int inLogLevel) {
        inLogLevel <= 9 ? mLogLevel = inLogLevel : mLogLevel = 9;
    }

private:
    static unsigned int mLogLevel;
};