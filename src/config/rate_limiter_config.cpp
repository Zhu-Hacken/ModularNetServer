#include "rate_limiter_config.h"

RateLimiterConfig::RateLimiterFunc RateLimiterConfig::m_limiter_func = nullptr;

void RateLimiterConfig::setRateLimiterFunc(RateLimiterFunc func) {
    m_limiter_func = func;
}

void RateLimiterConfig::registerAllRateLimiter() {
    if (m_limiter_func) m_limiter_func();
    // 防止单个 IP 请求爆刷
    // RateLimiter::getInstance().registerLimitRule("access_ip", 20, 10);
    // // 登录失败限流（防爆破登录）
    // RateLimiter::getInstance().registerLimitRule("login_ip", 5, 180);
}