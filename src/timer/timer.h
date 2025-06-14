#pragma once

#include <functional>
#include <chrono>

class TimerNode {
public:

    TimerNode(int fd, std::function<void()> cb, int timeout_ms);

    // 获取到期时间（用于排序）
    std::chrono::steady_clock::time_point getExpireTime() const;
    // 是否已过期
    bool isExpired() const;
    // 执行定时器任务
    void runCallback() const;
    // 获取关联的fd
    int getFd() const;

private:
    int m_fd;                                       // 关联连接的fd
    std::function<void()> m_callback;               // 到期回调函数
    std::chrono::steady_clock::time_point m_expire; // 到期时间
};