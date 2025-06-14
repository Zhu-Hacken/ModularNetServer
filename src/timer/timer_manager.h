#pragma once

#include <functional>
#include "timer.h"
#include <memory>
#include <queue>
#include <vector>
#include <unordered_map>
#include <mutex>

class TimerManager {
public:
    TimerManager() = default;
    ~TimerManager() = default;

    // 添加/更新定时器
    void addTimer(int id, std::function<void()> cb, int timeout_ms);
    
    // 检查并触发过期定时器
    void tick();

    // 定时器是否过期
    bool isExpired(int id);

private:
    // 比较两个定时器哪个先到期（小根堆）
    struct TimerCmp {
        bool operator()(const std::shared_ptr<TimerNode>& a, const std::shared_ptr<TimerNode>& b) const {
            return a->getExpireTime() > b->getExpireTime();
        }
    };

    // 定时器最小堆
    std::priority_queue<
        std::shared_ptr<TimerNode>,                 
        std::vector<std::shared_ptr<TimerNode>>,    
        TimerCmp
    > m_timer_heap;

    // fd -> TimerNode 的映射
    std::unordered_map<int, std::shared_ptr<TimerNode>> m_timer_map;
    std::mutex m_mutex;
};


