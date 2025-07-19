#pragma once
#include "log.h"

// 简化调用接口

// DEBUG 
inline void LOG_DEBUG(const std::string& msg) {
    if(!Log::getInstance().isClosed()){
        Log::getInstance().write(Log::DEBUG, msg);
    }
}

// INFO
inline void LOG_INFO(const std::string& msg) {
    if(!Log::getInstance().isClosed()){
        Log::getInstance().write(Log::INFO, msg);
    }
}

// WARN
inline void LOG_WARN(const std::string& msg) {
    if(!Log::getInstance().isClosed()){
        Log::getInstance().write(Log::WARN, msg);
    }
}

// ERROR
inline void LOG_ERROR(const std::string& msg) {
    if(!Log::getInstance().isClosed()){
        Log::getInstance().write(Log::ERROR, msg);
    }
}

// 改用可变参数宏，只有日志开启时才格式化字符串

#define LOG_DEBUGF(fmt, ...) \
    do { if (!Log::getInstance().isClosed()) Log::getInstance().writef(Log::DEBUG, fmt, ##__VA_ARGS__); } while(0)

#define LOG_INFOF(fmt, ...)  \
    do { if (!Log::getInstance().isClosed()) Log::getInstance().writef(Log::INFO, fmt, ##__VA_ARGS__); } while(0)

#define LOG_WARNF(fmt, ...)  \
    do { if (!Log::getInstance().isClosed()) Log::getInstance().writef(Log::WARN, fmt, ##__VA_ARGS__); } while(0)    

#define LOG_ERRORF(fmt, ...) \
    do { if (!Log::getInstance().isClosed()) Log::getInstance().writef(Log::ERROR, fmt, ##__VA_ARGS__); } while(0)

