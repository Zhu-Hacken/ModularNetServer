#include "websocket_conn.h"
#include "log/logs.h"
#include <unistd.h>
#include "util/utils.h"
#include "session/session_manager.h"
#include "websocket_conn_manager.h"
#include <regex>

WebSocketConn::WebSocketConn() : m_status(WSStatus::HANDSHAKE) {}

const std::string WS_BASE_TEXT = "[WebSocketConn] ";

void WebSocketConn::init(int sockfd, const sockaddr_in& addr) {
    if (m_sockfd != -1 && sockfd == m_sockfd && !m_is_closed ) {
        LOG_DEBUG(WS_BASE_TEXT + "跳过已初始化连接fd = " + std::to_string(sockfd));
        return;
    } 
    m_sockfd = sockfd;
    m_address = addr;
    m_is_closed = false;
    m_status = WSStatus::HANDSHAKE;

    m_read_buf.clear();
    m_write_buf.clear();
}

void WebSocketConn::closeConn() {
    if ( !m_is_closed.exchange(true)) {
        return;
    }
    if (m_sockfd != -1) {
        LOG_INFO(WS_BASE_TEXT + "WebSocketConn closed: fd = " + std::to_string(m_sockfd));
        close(m_sockfd);
        WebsocketConnManager::getInstance().removeFd(m_sockfd);
        m_sockfd = -1;
    }
}

// 是否已关闭连接
bool WebSocketConn::isClosed() {
    return false;
}

int WebSocketConn::getSockFd() const {
    return m_sockfd;
}

sockaddr_in WebSocketConn::getAddr() const {
    return m_address;
}

bool WebSocketConn::getKeepAlive() const {
    return true;   // WebSocket 不默认使用 HTTP Keep-Alive
}

bool WebSocketConn::read() {
    LOG_INFO(WS_BASE_TEXT + "进入 read()，当前触发模式 = " + (m_trig_mode == ServerConfig::ET ? "ET" : "LT"));
    if (m_sockfd == -1) {
        LOG_ERROR(WS_BASE_TEXT + "尝试对无效连接进行 read()，fd = -1");
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
            LOG_ERROR(WS_BASE_TEXT + "recv() failed, errno = " + std::to_string(errno));
            return false;
        } else if (bytes_read == 0) {
            // 客户端关闭连接
            return false;
        }
        m_read_buf.append(buffer, bytes_read);

        LOG_DEBUG(WS_BASE_TEXT + "read from fd = " + std::to_string(m_sockfd)
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
                LOG_ERROR(WS_BASE_TEXT + "recv() failed, errno = " + std::to_string(errno));
                return false;
            } else if (bytes_read == 0) {
                // 客户端关闭连接
                return false;
            }
            m_read_buf.append(buffer, bytes_read);
        }
        LOG_DEBUG(WS_BASE_TEXT + "read from fd = " + std::to_string(m_sockfd)
                    + ": \n" + m_read_buf);
        return true;
    }
    // 不明触发模式
    return false;
}

WriteStatus WebSocketConn::write() {
    if (m_sockfd == -1 || m_is_closed) {
        LOG_ERROR(WS_BASE_TEXT + "write(): socket 无效或连接已关闭");
        return WriteStatus::ERROR;
    }

    int write_buf_total_sent = 0;
    size_t to_write = m_write_buf.size();
    while (write_buf_total_sent < to_write) {
        int sent = send(m_sockfd, m_write_buf.data() + write_buf_total_sent, to_write - write_buf_total_sent, 0);
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                LOG_WARN(WS_BASE_TEXT + "write(): 非阻塞写缓冲区满，等待下一轮 EPOLLOUT");
                return WriteStatus::AGAIN;
            }
            LOG_ERROR(WS_BASE_TEXT + "write(): send() 出错，errno = " + std::to_string(errno));
            return WriteStatus::ERROR;
        }
        write_buf_total_sent += sent;
    }
    LOG_INFO( WS_BASE_TEXT + "响应发送成功，共" + std::to_string(write_buf_total_sent) + "字节");
    m_write_buf.clear();
    return WriteStatus::OK;

}

// 处理数据
bool WebSocketConn::process() {
    LOG_INFO(WS_BASE_TEXT + "Processing fd = " + std::to_string(m_sockfd) + ".");
    if (m_status == WSStatus::HANDSHAKE) {
        if (!handleHandshake()) return false;
        const SessionId sessionId = extractSessionIdFromRequest(m_read_buf);
        if ( !sessionId.empty()) {
            WebsocketConnManager::getInstance().bindSession(m_sockfd, sessionId);
            LOG_INFO(WS_BASE_TEXT + "绑定 sessionId: " + sessionId + " 到 fd = " + std::to_string(m_sockfd));
        }
        else {
            LOG_WARN(WS_BASE_TEXT + "未在请求中找到 sessionId");
        }
        // 清空缓冲区
        m_read_buf.clear();
    }
    else if (m_status == WSStatus::CONNECTED) {
        std::string msg;
        
        if (!parseWebSocketFrame(msg)) {
            LOG_WARN(WS_BASE_TEXT + "parseWebSocketFrame 失败或帧不完整");
            return false;
        }

        LOG_INFO(WS_BASE_TEXT + "收到客户端消息：" + msg);

        // 简单回声
        std::string reply = "你说的是：" + msg;
        sendTextFrame(reply);
    } else {
        LOG_ERROR(WS_BASE_TEXT + "未知状态！");
        return false;
    }
    if (m_write_cb) m_write_cb(m_sockfd);
    return true;
}

// 设置注册写事件的回调函数
void WebSocketConn::setEpollWriteCallback(EpollWriteCallback cb) {
    m_write_cb = std::move(cb);
}

// 解析 HTTP 握手，构造 Webocket 应答
bool WebSocketConn::handleHandshake() {
    /*
        客户端发送内容：
            GET /chat HTTP/1.1\r\n
            Host: localhost\r\n
            Upgrade: websocket\r\n
            Connection: Upgrade\r\n
            Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n
            Sec-WebSocket-Version: 13\r\n
            \r\n
    */
    /*
        服务器响应：
            HTTP/1.1 101 Switching Protocols\r\n
            Upgrade: websocket\r\n
            Connection: Upgrade\r\n
            Sec-WebSocket-Accept: <base64(sha1(key + GUID))>\r\n
            \r\n
    */
    LOG_INFO(WS_BASE_TEXT + "开始处理 WebSocket 握手，m_sockfd = " + std::to_string(m_sockfd));

    if (m_status == WSStatus::CONNECTED) {
        LOG_WARN(WS_BASE_TEXT + "已完成握手，不应该再次处理");
        return false;
    }

    // 检查是否包含完整 HTTP 请求（以"\r\n\r\n"结尾）
    size_t header_end = m_read_buf.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        LOG_WARN(WS_BASE_TEXT + "握手请求不完整，等待更多数据");
        return false;
    }

    // 查找Sec-WebSocket-Key
    const std::string key_prefix = "Sec-WebSocket-Key: ";
    size_t key_pos = m_read_buf.find(key_prefix);
    if (key_pos == std::string::npos) {
        LOG_ERROR(WS_BASE_TEXT + "握手请求缺少 Sec-WebSocket-Key");
        return false;
    }

    size_t key_start = key_pos + key_prefix.length();
    size_t key_end = m_read_buf.find("\r\n", key_start);
    if (key_end == std::string::npos) {
        LOG_ERROR(WS_BASE_TEXT + "握手请求中 Sec-WebSocket-Key 解析失败");
        return false;
    }

    std::string client_key = m_read_buf.substr(key_start, key_end - key_start);
    LOG_DEBUG(WS_BASE_TEXT + "解析得到客户端 Sec-WebSocket-Key: " + client_key);

    // 生成 Sec-WebSocket-Accept
    std::string accept_key = computeAcceptKey(client_key);

    // 构造响应头
    std::string response = 
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: " + accept_key + "\r\n\r\n";

    m_write_buf += response;
    m_status = WSStatus::CONNECTED;
    // LOG_INFO(WS_BASE_TEXT + "WebSocket 握手成功，发送响应头完成");
    LOG_INFO(WS_BASE_TEXT + "WebSocket 握手响应头构建完毕");
    return true;
}

std::string WebSocketConn::computeAcceptKey(const std::string& client_key) {
    const std::string GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string to_hash = client_key + GUID;

    std::string sha1_result = CryptoUtils::sha1(to_hash);
    std::string accept_key = CryptoUtils::base64Encode(sha1_result);

    LOG_DEBUG(WS_BASE_TEXT + "client_key = " + client_key + ", accept_key = " + accept_key);
    return accept_key;

}

void WebSocketConn::sendTextFrame(const std::string& msg) {
    std::string frame;

    const size_t payload_len = msg.size();
    frame.push_back(0x81);

    if (payload_len <= 125) {
        frame.push_back(static_cast<char>(payload_len));
    } else if (payload_len <= 65535) {
        frame.push_back(126);
        frame.push_back((payload_len >> 8) & 0xFF );
        frame.push_back(payload_len & 0xFF);
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; --i) {
            frame.push_back((payload_len >> (i * 8)) & 0xFF);
        }
    }

    frame += msg;
    m_write_buf += frame;

    LOG_INFO(WS_BASE_TEXT + "sendTextFrame() 构造 WebSocket 帧成功，原始消息长度 = " + std::to_string(payload_len) +
             "，总帧长度 = " + std::to_string(frame.size()));
}

bool WebSocketConn::parseWebSocketFrame(std::string& message) {
    if (m_read_buf.empty()) {
        return false;
    }

    const size_t min_header_size = 2;
    if ( m_read_buf.size() < min_header_size) {
        // 不够基本头部
        return false;
    }

    const unsigned char* data = reinterpret_cast<const unsigned char*>(m_read_buf.data());
    bool fin = (data[0] & 0x80) != 0;
    uint8_t opcode = data[0] & 0x0f;

    bool masked = (data[1] & 0x80) != 0;
    uint64_t payload_len = data[1] & 0x7f;

    size_t pos = 2; // 当前解析位置

    if (payload_len == 126) {
        if (m_read_buf.size() < pos + 2) return false;
        payload_len = (data[pos] << 8) | data[pos + 1];
        pos += 2;
    } else if (payload_len == 127) {
        if (m_read_buf.size() < pos + 8) return false;
        payload_len = 0;
        for (int i = 0; i < 8; ++i) {
            payload_len = ( payload_len << 8) | data[pos + i];
        }
        pos += 8;
    }

    if (!masked) {
        LOG_WARN(WS_BASE_TEXT + "客户端数据未加掩码，不合法连接");
        return false;
    }

    if (m_read_buf.size() < pos + 4 + payload_len) {
        return false;
    }

    // 获取4字节掩码
    const unsigned char* mask_key = data + pos;

    pos += 4;

    // 获取原始 payload 数据
    const unsigned char* encoded_payload = data + pos;

    std::string decoded;
    decoded.reserve(payload_len);
    for (uint64_t i = 0; i < payload_len; ++i) {
        decoded.push_back(encoded_payload[i] ^ mask_key[i % 4]);
    }

    // 提取成功，输出 message
    message = decoded;

    // 移除已处理数据
    m_read_buf.erase(0, pos + payload_len);

    return true;
}

std::string WebSocketConn::extractSessionIdFromRequest(const std::string& request) {
    std::smatch match;
    std::regex session_regex(R"(GET\s+/\?sessionId=([^ ]+))");

    if (std::regex_search(request, match, session_regex)) {
        return match[1].str();
    }
    return "";
}