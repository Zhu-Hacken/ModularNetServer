#include "timer.h"

TimerNode::TimerNode(int fd, std::function<void()> cb, int timeout_ms)
                : m_fd(fd), m_callback(cb)
{
    auto now = std::chrono::steady_clock::now();
    m_expire = now + std::chrono::milliseconds(timeout_ms);
}

std::chrono::steady_clock::time_point TimerNode::getExpireTime() const
{
    return m_expire;
}

bool TimerNode::isExpired() const
{
    auto now = std::chrono::steady_clock::now();
    return now >= m_expire;
}

void TimerNode::runCallback() const
{
    if(m_callback) {
        m_callback();
    }
}

int TimerNode::getFd() const
{
    return m_fd;
}
