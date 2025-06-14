#include "rate_limiter_config.h"
#include "util/rate_limiter.h"

void RateLimiterConfig::registerAllRateLimiter() {
    // 防止单个 IP 请求爆刷
    RateLimiter::getInstance().registerLimitRule("access_ip", 20, 10);
    // 登录失败限流（防爆破登录）
    RateLimiter::getInstance().registerLimitRule("login_ip", 5, 180);
}