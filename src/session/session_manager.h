#pragma once

#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include "timer/timer_manager.h"

// Session 类型定义
using SessionId = std::string;
using SessionData = std::unordered_map<std::string, std::string>;
using TimePoint = std::chrono::steady_clock::time_point;

// Session 对象
struct Session {
    SessionData data;
    TimePoint expireTime;
};

class SessionManager {
public:
    static SessionManager& getInstance();

    // 创建 Session，返回 session_id
    SessionId createSession(int timeout_ms = 1800000);
    // 检查SessionId是否有效
    bool isSessionIdValid(const SessionId& sessionId);
    // 检查SessionId是否过期
    bool isSessionIdExpired(const SessionId& sessionId);
    // 设置/获取/删除 session 数据
    void set(const SessionId& id, const std::string& key, const std::string& value);
    std::string get(const SessionId& id, const std::string& key);
    bool exists(const SessionId& id);
    void removeSession(const SessionId& id);

    // 刷新 session 的过期时间
    void refresh(const SessionId& id, int timeout_ms = 1800000);

private:
    SessionManager() = default;
    ~SessionManager() = default;

    // 禁止拷贝
    SessionManager(const SessionManager&) = delete;
    SessionManager operator=(const SessionManager&) = delete;

    // 定时器回调：清除过期 session
    void cleanSession(const SessionId& id);

private:
    std::unordered_map<SessionId, Session> m_sessions;
    std::mutex m_mutex;
    TimerManager m_timer_manager;

    // === 定时器相关 ===
    std::unordered_map<SessionId, int>  m_session_timers;
    static int timer_seed;  // 下一个可用的逻辑 定时器id
};