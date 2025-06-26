#pragma once
#include "session/session_manager.h"
#include <mutex>

class WebsocketConnManager {

public:
    static WebsocketConnManager& getInstance();

    // 绑定 fd 和 sessionId
    bool bindSession( int fd, const SessionId& sessionId);
    // 向 sessionId 的连接发送信息
    bool sendToSession(const SessionId& sessionId, const std::string& msg);
    // 移除 fd
    void removeFd(int fd);
    // 检查 session 是否有效
    bool isSessionAlive(const SessionId& sessionId);
    // 根据 fd 返回 sessionId
    SessionId getSessionIdByFd(const int fd);

private:
    std::mutex  m_mutex;
    std::unordered_map<SessionId, int>  m_session_to_fd;
    std::unordered_map<int, SessionId> m_fd_to_session;

};