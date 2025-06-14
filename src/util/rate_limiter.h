#pragma once
#include <unordered_map>
#include <string>
#include <mutex>
#include <chrono>

// 单个请求实体的信息（限流记录）
struct RequestInfo {
    int count;  // 当前统计窗口内的请求次数
    std::chrono::steady_clock::time_point last_reset_time;    // 上一次窗口重置时间
};

class RateLimiter {
public:
    // 初始化限流器（如默认启用设置等）
    void init(bool rate_limiter_close);             
    /*
        注册限流规则（type维度）：
            type：限流类型（如 login_fail, api_access）
            threshold：时间窗口内最多允许的次数
            window_sec：时间窗口大小（秒）
    */
    void registerLimitRule(const std::string& type, int max_count, int window_sec);

    // 单例访问
    static RateLimiter& getInstance();

    // 判断某个 type + key 是否触发限流（如 login_fail + IP）
    bool isLimit(const std::string& type, const std::string& key);
    // 清除某个 type + key 的历史记录
    void clear(const std::string& type, const std::string& key);
    // 判断某个限流类型是否启用
    bool istypeEnable(const std::string& type);
    // 限制器是否关闭
    bool isLimiterClose();
    // === 旧 ===
    // 某个ip是否限制
    // bool isLimit(const std::string& ip);

private:
    std::mutex m_mutex;
    std::unordered_map<std::string, RequestInfo> m_ipLimitMap;
    std::unordered_map<std::string, std::unordered_map<std::string, RequestInfo>> m_typeLimitMap; // 按类型区分的限流记录：type -> (key -> RequestInfo)
    std::unordered_map<std::string, int> m_typeTimeWindowMap;  // 每种类型对应的时间窗口（秒）
    std::unordered_map<std::string, int> m_typeMaxCountMap;  // 每种类型对应的最大请求次数阈值
    std::unordered_map<std::string, bool> m_typeLimiterEnableMap;  // 每种类型是否启用限流器（如部分功能可关闭）
    bool m_rate_limiter_close;                         // 是否开启频率限制器

    // === 旧 ===
    // const static int m_timeWindow;                   // 时间窗口大小
    // static const int m_maxRequestCount;             // 窗口内最大请求次数
};