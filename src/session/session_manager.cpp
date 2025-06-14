#include "session_manager.h"
#include <thread>
#include <sstream>
#include "util/utils.h"
#include "log/logs.h"


int SessionManager::timer_seed = 1000000;
const std::string BASE_TEXT = "[SessionManager] ";

SessionManager& SessionManager::getInstance() {
    static SessionManager instance;
    return instance;
}

SessionId SessionManager::createSession(int timeout_ms) {
    /*
    session示例：sess_时间戳_线程id
    */

    std::lock_guard<std::mutex> lock(m_mutex);

    // 生成 session ID
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    auto tid = std::this_thread::get_id();
    std::ostringstream oss;
    oss << "sess_" << now << "_" << tid;
    SessionId session_id = oss.str();

    // 构造 Session 对象
    Session session;
    session.expireTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    m_sessions[session_id] = std::move(session);

    int timer_id = timer_seed++;

    m_session_timers[session_id] = timer_id;

    // 注册定时器
    m_timer_manager.addTimer(timer_id, [this, session_id]() {
        this->cleanSession(session_id);
    }, timeout_ms);

    return session_id;
}

// 检查SessionId是否有效
bool SessionManager::isSessionIdValid(const SessionId& sessionId) {
    return exists(sessionId);
}

// 检查SessionId是否过期
bool SessionManager::isSessionIdExpired(const SessionId& sessionId) {
    return m_timer_manager.isExpired(m_session_timers[sessionId]);
}

void SessionManager::set(const SessionId& id, const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sessions.find(id);
    if (it != m_sessions.end()) it->second.data[key] = value;
}

std::string SessionManager::get(const SessionId& id, const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sessions.find(id);
    if (it != m_sessions.end()) {
        auto key_it = it->second.data.find(key);
        if (key_it != it->second.data.end()) {
            return key_it->second;
        }
    }
    return "";
}

bool SessionManager::exists(const SessionId& id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_sessions.find(id) != m_sessions.end();
}

void SessionManager::removeSession(const SessionId& id) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 删除 Session 对象
    auto it_s = m_sessions.find(id);
    if (it_s != m_sessions.end()) {
        m_sessions.erase(it_s);
    }

    // 删除定时器
    auto it_st = m_session_timers.find(id);
    if (it_st != m_session_timers.end()) {
        m_session_timers.erase(it_st);
    }
}

void SessionManager::refresh(const SessionId& id, int timeout_ms) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_sessions.find(id);
    if (it == m_sessions.end()) return;

    // 更新过期时间
    it->second.expireTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);

    // 获取定时器id
    auto it_timer = m_session_timers.find(id);
    if (it_timer == m_session_timers.end()) return;

    int timer_id = it_timer->second;

    // 重新添加定时器
    m_timer_manager.addTimer(timer_id, [this, id](){
        this->cleanSession(id);
    }, timeout_ms);
    LOG_INFO(BASE_TEXT + "续期 session_id = " + id);
}

void SessionManager::cleanSession(const SessionId& id) {
    removeSession(id);
    LOG_INFO(BASE_TEXT + "Session expired and cleaned: " + id);
}