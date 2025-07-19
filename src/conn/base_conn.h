#pragma once
#include <string>
#include <netinet/in.h>
#include <functional>
#include <atomic>
#include "config/configs.h"

using EpollWriteCallback = std::function<void(int fd)>;

enum class WriteStatus {
        OK,             // 全部写完
        AGAIN,          // 等待缓冲区可写
        ERROR           // 错误
    };

class BaseConn {
public:
    BaseConn() :m_sockfd(-1), m_address{}, m_is_closed(false) {}
    // 虚析构函数，确保多态删除安全
    virtual ~BaseConn() = default;

    // 初始化连接
    virtual void init(int sockfd = -1, const sockaddr_in& addr = sockaddr_in()) = 0;
    // 关闭连接
    virtual void closeConn() = 0;
    // 是否已关闭连接
    virtual bool isClosed() = 0;
    // 读数据
    virtual bool read() = 0;
    // 发送响应
    virtual WriteStatus write() = 0;  
    // 处理数据
    virtual bool process() = 0;
    // 设置注册写事件的回调函数
    virtual void setEpollWriteCallback(EpollWriteCallback cb) = 0;
    // 执行注册写事件的回调
    bool runEpollWriteCallback();

    // === Getter ===
    // 获取文件描述符
    virtual int getSockFd() const = 0;
    // 获取远程地址信息（如 sockaddr_in）
    virtual sockaddr_in getAddr() const = 0;
    virtual bool getKeepAlive() const = 0;

    static int m_trig_mode;  // 触发模式：0 = LT（水平触发），1 = ET（边沿触发）
    static int m_actor_model;  // 触发模式：0 = Proactor，1 = Reactor

protected:
    int m_sockfd;                   // 当前连接的socket文件描述符
    sockaddr_in m_address;          // 客户端地址
    std::atomic<bool> m_is_closed;  // 该连接是否关闭
    EpollWriteCallback m_write_cb;  // 注册写事件的回调函数

};

// int BaseConn::m_trig_mode = ServerConfig::ET;