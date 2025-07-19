#include "thread_pool.h"
#include "log/log_utils.h"
#include "util/sys_utils.h"

ThreadPool::ThreadPool(size_t thread_count) : m_stop(false)
{
    LOG_INFO("[ThreadPool] 初始化线程池，共 " + std::to_string(thread_count) + " 个线程");
    for(size_t i = 0; i < thread_count; i++) { 
        m_threads.emplace_back([this](){
            this->worker();
        });
    }
}

ThreadPool::~ThreadPool() {}

void ThreadPool::addTask(const std::function<void()> &task)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_tasks.push(task); // 入队
        LOG_DEBUGF("[ThreadPool] 添加任务，当前任务数 = %d", m_tasks.size());
    }
    m_cv.notify_one();  // 通知一个等待线程
}

void ThreadPool::shutdown() {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_stop = true;
    }
    m_cv.notify_all();  // 唤醒所有线程

    for (std::thread &worker: m_threads) {
        if (worker.joinable()) {
            worker.join();  // 等待线程退出
        }
    }
    LOG_INFO("[ThreadPool] 线程池已关闭。");
}

void ThreadPool::worker()
{
    std::string base_text = "[ThreadPool] [Thread-"+ SysUtils::getThreadIdStr() +"] ";
    LOG_INFO(base_text + "线程启动");
    while(!m_stop) {
        std::function<void()> task;

        {   // 互斥锁作用范围
            std::unique_lock<std::mutex> lock(m_mutex);
            LOG_DEBUGF( + "%s线程等待任务中...", base_text.c_str());
            // 等待任务或退出
            m_cv.wait(lock, [this]() {
                return m_stop || !m_tasks.empty();
            });

            // 如果是退出状态，直接return
            if(m_stop && m_tasks.empty()) return;
            LOG_DEBUGF( "%s线程获取任务，准备执行", base_text.c_str());
            // 拿出任务：移动构造，性能更优
            task = std::move(m_tasks.front());
            m_tasks.pop();
        }   // 互斥锁作用范围结束，自动释放锁

        // 执行任务
        task();
    
    }
}
