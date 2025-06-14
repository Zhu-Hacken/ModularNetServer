#pragma once

#include <functional>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ThreadPool {
public:
    ThreadPool(size_t thread_count = 8);
    ~ThreadPool();

    void addTask(const std::function<void()>& task);
    void shutdown();
    
private:
    void worker();  // 每个线程要执行的循环逻辑

    std::vector<std::thread> m_threads;             // 工作线程
    std::queue<std::function<void()>> m_tasks;      // 任务队列
    std::mutex m_mutex;                             // 互斥锁保护任务队列
    std::condition_variable m_cv;                   // 条件变量用于线程阻塞/唤醒
    std::atomic<bool> m_stop;                       // 控制线程退出，多线程共享原子变量安全访问

};