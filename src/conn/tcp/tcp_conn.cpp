#include "tcp_conn.h"
#include "logs.h"

const std::string BASE_TEXT = "[TcpConn] ";

TcpConn::TcpConn() : m_status(TcpStatus::HANDSHAKE) {}

TcpConn::~TcpConn() {
    closeConn();
}

// 初始化连接
void TcpConn::init(int sockfd, const sockaddr_in& addr) {
    if (m_sockfd != -1 && sockfd == m_sockfd && !m_is_closed) {
        LOG_DEBUG(BASE_TEXT + "跳过已初始化连接fd = " + std::to_string(m_sockfd));
        return;
    }

    m_sockfd = sockfd;
    m_address = addr;
    m_is_closed = false;
    m_status = TcpStatus::HANDSHAKE;

    // 清空缓冲
    m_read_buf.clear();
    m_write_buf.clear();
    LOG_INFO(BASE_TEXT + "初始化连接fd = " + std::to_string(m_sockfd));
}

// 关闭连接
void TcpConn::closeConn() {}
// 是否已关闭连接
bool TcpConn::isClosed() {}

// 读数据
bool TcpConn::read() {
    LOG_INFO(BASE_TEXT + "进入 read()，当前触发模式 = " + (m_trig_mode == ServerConfig::ET ? "ET" : "LT"));
    if (m_sockfd == -1) {
        LOG_ERROR(BASE_TEXT + "尝试对无效连接进行 read()，fd = -1");
        return false;
    }

    char buffer[4096];
    int bytes_read = 0;

    if (m_trig_mode == ServerConfig::LT) {
        bytes_read = recv(m_sockfd, buffer, sizeof(buffer), 0);
        if (bytes_read < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 非阻塞读完了
                return false;
            }
            LOG_ERROR(BASE_TEXT + "recv() failed, errno = " + std::to_string(errno));
            return false;
        } else if (bytes_read == 0) {
            // 客户端关闭连接
            return false;
        }
        m_read_buf.append(buffer, bytes_read);

        LOG_DEBUG(BASE_TEXT + "read from fd = " + std::to_string(m_sockfd)
                    + ": \n" + m_read_buf);
        return true;
    } else if (m_trig_mode == ServerConfig::ET) {
        // ET模式，必须一次性读取完缓冲区
        while (true) {
            bytes_read = recv(m_sockfd, buffer, sizeof(buffer), 0);
            if (bytes_read < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // 非阻塞读完了
                    break;
                }
                LOG_ERROR(BASE_TEXT + "recv() failed, errno = " + std::to_string(errno));
                return false;
            } else if (bytes_read == 0) {
                // 客户端关闭连接
                return false;
            }
            m_read_buf.append(buffer, bytes_read);
        }
        LOG_DEBUG(BASE_TEXT + "read from fd = " + std::to_string(m_sockfd)
                    + ": \n" + m_read_buf);
        return true;
    }
    // 不明触发模式
    return false;
}

// 发送响应
WriteStatus TcpConn::write() {}  

// 处理数据
bool TcpConn::process() {
    // LOG_INFO(BASE_TEXT + "Processing fd = " + std::to_string(m_sockfd) + ".");
    // if (m_status == TcpStatus::HANDSHAKE) {
    //     if (!handleHandshake()) return false;
    //     // 清空缓冲区
    //     m_read_buf.clear();
    // }
    // else if (m_status == TcpStatus::CONNECTED) {
    //     std::string msg;
        
    //     if (!parseTcpFrame(msg)) {
    //         LOG_WARN(BASE_TEXT + "parseTcpFrame 失败或帧不完整");
    //         return false;
    //     }

    //     LOG_INFO(BASE_TEXT + "收到客户端消息：" + msg);
    //     // TODO: 路由

    //     // 简单回声
    //     std::string reply = "你说的是：" + msg;
    //     sendTextFrame(reply);
    // } else {
    //     LOG_ERROR(BASE_TEXT + "未知状态！");
    //     return false;
    // }

    // if (m_write_cb) m_write_cb(m_sockfd);
    // return true;
}

// 设置注册写事件的回调函数
void TcpConn::setEpollWriteCallback(EpollWriteCallback cb) {}

// === Getter ===
// 获取文件描述符
int TcpConn::getSockFd() const {}

// 获取远程地址信息（如 sockaddr_in）
sockaddr_in TcpConn::getAddr() const {}

bool TcpConn::getKeepAlive() const {}