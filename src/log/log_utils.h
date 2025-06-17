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