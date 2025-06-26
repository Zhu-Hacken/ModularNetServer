#pragma once
#include "conn/base_conn.h"
#include "third_party/nlohmann/json.hpp"
 
using Json = nlohmann::json;


class WebSocketConn : public BaseConn {
public:    
    enum class WSStatus {
        HANDSHAKE,
        CONNECTED
    };
    
    public:
    WebSocketConn();
    ~WebSocketConn() override = default;

    // 初始化连接
    virtual void init(int sockfd = -1, const sockaddr_in& addr = sockaddr_in()) override;
    // 关闭连接
    virtual void closeConn() override;
    // 是否已关闭连接
    virtual bool isClosed() override;
    // 读数据
    virtual bool read() override;
    // 发送响应
    virtual WriteStatus write() override;  
    // 处理数据
    virtual bool process() override;
    // 设置注册写事件的回调函数
    virtual void setEpollWriteCallback(EpollWriteCallback cb) override;
    
    // === Getter ===
    // 获取文件描述符
    virtual int getSockFd() const override;
    // 获取远程地址信息（如 sockaddr_in）
    virtual sockaddr_in getAddr() const override;
    virtual bool getKeepAlive() const override;

    // 发送数据帧
    void sendTextFrame(const std::string& msg);
    
private:
    WSStatus m_status;
    
    std::string m_read_buf;
    std::string m_write_buf;
    
    // 解析 HTTP 握手，构造 Webocket 应答
    bool handleHandshake();     
    std::string computeAcceptKey(const std::string& client_key);
    
    // 解析数据帧
    bool parseWebSocketFrame(std::string& message);

    // 握手后从请求中提取sessionId
    std::string extractSessionIdFromRequest(const std::string& request);
};