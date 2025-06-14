#include "rate_limiter.h"
#include "util/utils.h"
#include "log/logs.h"

const std::string BASE_TEXT = "[RateLimiter] ";

void RateLimiter::init(bool rate_limiter_close){
    m_rate_limiter_close = rate_limiter_close;
}            

void RateLimiter::registerLimitRule(const std::string& type, int max_count, int window_sec) {
    // 频率限制器是否已经关闭
    if (m_rate_limiter_close) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_typeLimitMap.insert({type,{}});
    m_typeMaxCountMap[type] = max_count;
    m_typeTimeWindowMap[type] = window_sec;
    m_typeLimiterEnableMap[type] = true;
}

RateLimiter& RateLimiter::getInstance() {
    static RateLimiter instance;
    return instance;
}

bool RateLimiter::isLimit(const std::string& type, const std::string& key) {
    // 频率限制器是否已经关闭
    if (m_rate_limiter_close) return false;

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it_type = m_typeLimitMap.find(type);
    // type是否具有频率限制器
    if (it_type == m_typeLimitMap.end()) return false;

    // 先查是否存在该key（ip、session、token等）
    auto it_key = it_type->second.find(key);
    auto now = std::chrono::steady_clock::now();

    if (it_key == it_type->second.end()) {
        it_type->second[key] = {1, now};
        return false;
    }
    
    // 判断是否在时间窗口内
    if (now - it_key->second.last_reset_time < std::chrono::seconds(m_typeTimeWindowMap[type])) {
        // 判断是否超过窗口阈值
        if (it_key->second.count >= m_typeMaxCountMap[type]) {
            LOG_INFO(BASE_TEXT + type + " - " + key + " 请求过于频繁，已被限制。");
            return true;
        }
        ++it_key->second.count;
    }
    else {
        it_key->second.count = 1;
        it_key->second.last_reset_time = now;
        return false;
    }
    return false;
}

void RateLimiter::clear(const std::string& type, const std::string& key) {
    // 频率限制器是否已经关闭
    if (m_rate_limiter_close) return;
    std::lock_guard<std::mutex> lock(m_mutex);


    auto it_type = m_typeLimitMap.find(type);
    if (it_type == m_typeLimitMap.end()) return;

    auto it_key = it_type->second.find(key);
    if(it_key == it_type->second.end()) return;
    else it_type->second.erase(key);
}


bool RateLimiter::istypeEnable(const std::string& type) {
    // 频率限制器是否已经关闭
    if (m_rate_limiter_close) return false;

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it_type = m_typeLimitMap.find(type);
    if (it_type == m_typeLimitMap.end()) return false;

    auto it_enable = m_typeLimiterEnableMap.find(type);
    if (it_enable == m_typeLimiterEnableMap.end()) return false;
    else return it_enable->second;

}

bool RateLimiter::isLimiterClose() {
    return m_rate_limiter_close;
}

// bool RateLimiter::isLimit(const std::string& ip) {
//     if (m_rate_limiter_close) return false;

//     std::lock_guard<std::mutex> lock(m_mutex);

//     // 先查找 ip 对应的信息
//     auto it = m_ipLimitMap.find(ip);
//     auto now = std::chrono::steady_clock::now();
//     // 如果是新ip，则加入
//     if (it == m_ipLimitMap.end()) {
//         m_ipLimitMap[ip] = {1, now};
//         return false;
//     }


//     // 判断是否在窗口时间内
//     if (now - it->second.last_reset_time < std::chrono::seconds(m_timeWindow)) {
//         // 判断是否超过次数
//         if (it->second.count >= m_maxRequestCount) return true;
//         ++it->second.count;
//     }
//     else {  // 超过窗口时间，时间与计数重置
//         // 计数重置
//         it->second.count = 1;
//         it->second.last_reset_time = now;
//     }
//     return false;
// }