#pragma once
#include "util/rate_limiter.h"

class RateLimiterConfig {
public:
    using RateLimiterFunc = void(*)();
    static void setRateLimiterFunc(RateLimiterFunc func);
    static void registerAllRateLimiter();
private:
    static RateLimiterFunc m_limiter_func;    
};