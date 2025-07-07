#include "timer_manager.h"
#include "util/utils.h"
#include <iostream>
#include "log/logs.h"

void TimerManager::addTimer(int id, std::function<void()> cb, int timeout_ms, bool repeat)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!cb) {
        LOG_ERROR("[TimerManager] 提交了空的回调函数 id = " + std::to_string(id));
    }

    // 创建新的定时器（智能指针）
    auto timer = std::make_shared<TimerNode>(id, cb, timeout_ms, repeat);

    if (!timer) {
        LOG_ERROR("[TimerManager] make_shared 创建失败！");
    }

    // 更新映射（对于旧连接，该步骤会覆盖旧定时器）
    m_timer_map[id] = timer;
    // 注册定时器（加入堆中）
    m_timer_heap.push(timer);
}

void TimerManager::tick()
{
    std::lock_guard<std::mutex> lock(m_mutex);  // 上锁
    LOG_DEBUG("[TimerManager] -> tick() 进入，当前堆大小 = " + std::to_string(m_timer_heap.size()));
    while(!m_timer_heap.empty()) {
        auto timer = m_timer_heap.top();
        LOG_DEBUG("[TimerManager] -> 堆顶定时器地址: " + std::to_string(reinterpret_cast<uintptr_t>(timer.get())));
        if (!timer) {
            LOG_ERROR("[TimerManager] 空定时器，跳过");
            std::cout << ("[TimerManager] 空定时器，跳过") << std::endl;
            m_timer_heap.pop();
            continue;
        }
        
        int fd = timer->getFd();
        // LOG_DEBUG("[TimerManager] -> 调用 isExpired()");
        if (timer->isExpired()) {
            // 当前堆顶超时
            if (m_timer_map.count(fd) && m_timer_map[fd] == timer) {
                // 当前fd映射到该timer，那么需要触发回调
                timer->runCallback();   // 执行定时器动作
                if (timer->isRepeat()) {
                    // 更新到期时间
                    LOG_DEBUG("[TimerManager] Refresh fd = " + std::to_string(fd));
                    auto new_timer = std::make_shared<TimerNode>(fd, timer->getCallback(), timer->getInterval(), true);
                    m_timer_map[fd] = new_timer;
                    m_timer_heap.push(new_timer);
                } else {
                    LOG_INFO("[TimerManager] Try expire fd = " + std::to_string(fd) + " 有效，即将关闭");
                    m_timer_map.erase(timer->getFd());  
                }
            } 
            else {
                // 惰性删除，因此可能有些fd映射到新的timer，而当前timer属于废弃的旧timer，那么不触发回调
                LOG_DEBUG("[TimerManager] Try expire fd = " + std::to_string(fd) + " 无效，已被刷新，跳过");
            }

            m_timer_heap.pop(); 
        } 
        else {
            LOG_DEBUG("[TimerManager] -> 堆顶未过期，结束");
            break;  // 堆顶未超时，提前结束
        }
    }
}

bool TimerManager::isExpired(int id) {
    auto it = m_timer_map.find(id);
    if (it == m_timer_map.end()) return true;
    return it->second->isExpired();
}