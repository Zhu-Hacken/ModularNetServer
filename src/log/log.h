#pragma once

#include <string>
#include <fstream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>


class Log {
public:
    enum LogLevel {
        DEBUG,
        INFO,
        WARN,
        ERROR
    };

public:
    static bool m_close_log;                       // 是否关闭日志
    // 单例获取
    static Log& getInstance();  
    // 初始化日志文件
    void init( LogLevel level = INFO, const std::string& filename = "./log/server_log", bool enable_log = true);  
    // 写日志
    void write(LogLevel level, const std::string& message);         
    // 刷新日志缓冲区
    void flush();                                                   
    // 设置日志开关
    static void setLogEnabled(bool enabled) {
        m_close_log = !enabled;
    }
    // 日志是否已经关闭
    bool isClosed();
    // 禁用拷贝构造
    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;
    // 关闭日志
    void shutdown();


private:
    Log();
    ~Log();
    // 后台日志线程（异步消费日志队列）
    void logThread();   
    // 格式化每条日志（加时间戳等）
    std::string formatLog(LogLevel level, const std::string& message);
    // 日志等级转换字符串
    std::string levelToString(LogLevel level);


private:
    std::ofstream m_log_file;               // 文件流
    std::queue<std::string> m_log_queue;    // 待写入的日志队列
    std::mutex m_mutex;                     // 队列锁
    std::condition_variable m_cv;           // 条件变量：日志线程阻塞/唤醒
    std::atomic<bool> m_running;            // 是否正在运行
    std::thread m_thread;                   // 日志线程
    LogLevel m_level;                       // 当前日志等级阈值
};