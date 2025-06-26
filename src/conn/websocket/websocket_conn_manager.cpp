#include "websocket_conn_manager.h"
#include "log/logs.h"
#include "net/net_server.h"
#include "websocket_conn.h"
const std::string BASE_TEXT = "[WebsocketConnManager] ";

WebsocketConnManager& WebsocketConnManager::getInstance() {
    static WebsocketConnManager instance;
    return instance;
}

bool WebsocketConnManager::bindSession( int fd, const SessionId& sessionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_session_to_fd[sessionId] = fd;
    m_fd_to_session[fd] = sessionId;
    return true;
}

bool WebsocketConnManager::sendToSession(const SessionId& sessionId, const std::string& msg) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_session_to_fd.find(sessionId);
    if (it == m_session_to_fd.end()) return false;

    int fd = it->second;

    // 获取连接对象
    auto conn = NetServer::getInstance().getConn(fd);
    if (conn == nullptr) return false;
    auto ws_conn = std::dynamic_pointer_cast<WebSocketConn>(conn);
    if (!ws_conn || ws_conn->isClosed()) return false;

    ws_conn->sendTextFrame(msg);
    ws_conn->runEpollWriteCallback();
    return true;
}

void WebsocketConnManager::removeFd(int fd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_fd_to_session.find(fd);
    if (it != m_fd_to_session.end()) {
        m_session_to_fd.erase(it->second);
        m_fd_to_session.erase(it);
    }
}

bool WebsocketConnManager::isSessionAlive(const SessionId& sessionId) {
    return SessionManager::getInstance().isSessionIdValid(sessionId);
}

SessionId WebsocketConnManager::getSessionIdByFd(const int fd) {
    auto it = m_fd_to_session.find(fd);
    if ( it == m_fd_to_session.end()) return "";
    return it->second;
}