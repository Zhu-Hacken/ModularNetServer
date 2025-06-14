#pragma once
#include "timer/timer_manager.h"
#include <mutex>
#include <unordered_map>

using Token = std::string;

class TokenManager {
public:
    // 获取 TokenManager 单例
    static TokenManager& getInstance();
    // 根据用户名生成一个token
    Token createToken(std::string username, int timeout_ms = 1800000);
    // 检查token是否有效
    bool isTokenValid(const Token& token);
    // 检查token是否过期
    bool isTokenExpired(const Token& token);
    // 删除token
    void removeToken(const Token& token);
    // 从token中解析出用户名
    std::string getUsername(const Token& token);
    // 刷新token
    void refreshToken(const Token& token, int timeout_ms = 1800000);
    // token是否存在
    bool exist(const Token& token);

private:
    TokenManager() = default;
    ~TokenManager() = default;
    
    // 禁止拷贝
    TokenManager(const TokenManager&) = delete;
    TokenManager operator=(const TokenManager&) = delete;

    // 定时器回调
    void cleanToken(const Token& token);    

private:
    std::unordered_map<Token, std::string> m_token_username;    // token -> 用户映射表
    std::unordered_map<Token, int> m_token_timers;              // token -> 定时器id映射表
    std::mutex m_mutex;
    TimerManager m_timer_manager;   // 定时器

    static int timer_seed;
};