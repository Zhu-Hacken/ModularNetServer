#include "base_conn.h"

int BaseConn::m_trig_mode = ServerConfig::ET;
int BaseConn::m_actor_model = ServerConfig::Proactor;
bool BaseConn::runEpollWriteCallback() {
    if (!m_write_cb) return false;;
    m_write_cb(m_sockfd);
    return true;
}