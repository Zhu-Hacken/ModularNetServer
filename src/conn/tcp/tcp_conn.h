#pragma once
#include "base_conn.h"
#include <memory>

class TcpConn : public BaseConn, public std::enable_shared_from_this<TcpConn> {
public:
    enum class TcpStatus {
        INIT,       // 初始状态，未开始通信
        HANDSHAKE,  // 等待握手
        CONNECTED,  // 握手成功，通信状态
        CLOSED
    };
    TcpConn();
    ~TcpConn() override;

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

private:
    bool handleHandshake();

    bool m_keep_alive = true;

    std::string m_read_buf;
    std::string m_write_buf;

    TcpStatus m_status;
};