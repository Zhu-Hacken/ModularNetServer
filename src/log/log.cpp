#include "log.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <experimental/filesystem>
#include "util/utils.h"
#include <cstdarg>

namespace fs = std::experimental::filesystem;

bool Log::m_close_log = false;        // 默认打开日志

Log &Log::getInstance()
{
    static Log instance;
    return instance;
}

void Log::init(LogLevel level, const std::string &filename, bool enable_log)
{
    m_close_log = !enable_log;
    if (m_close_log) return;

    if(!fs::exists(SysUtils::getRootPath() + "/log")){
        fs::create_directories(SysUtils::getRootPath() + "/log");
    }

    // 获取当前时间并格式化为日志文件名
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm = *std::localtime(&now);

    std::string filepath = (SysUtils::getRootPath() + "/log/" + filename).c_str();

    std::ostringstream filename_stream;
    filename_stream << filepath << "_" << std::put_time(&tm, "%Y-%m-%d") << ".log";
    std::string log_filename = filename_stream.str();


    m_log_file.open(log_filename, std::ios::out | std::ios::app);   // 打开日志文件（追加模式）
    m_log_file.imbue(std::locale("en_US.UTF-8"));
    if(!m_log_file.is_open()) {
        std::cerr << ("[Log] 打开日志文件失败: " + log_filename);
        return;
    }

    m_level = level;        // 设置日志等级

    m_running = true;
    m_thread = std::thread(&Log::logThread, this);  // 启动日志线程
}

void Log::write(LogLevel level, const std::string &message)
{
    if(m_close_log) return;
    if(level >= m_level) {      // 如果日志等级符合要求
        std::string log_entry = formatLog(level, message);  // 格式化日志
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_log_queue.push(log_entry);    // 格式化的日志添加到队列
        }

        m_cv.notify_one();  // 唤醒日志队列，处理队列中的日志
    }
}

void Log::writef(LogLevel level, const char* fmt, ...) {
    if (m_close_log || level < m_level) return;

    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    write(level, std::string(buffer));
}


void Log::flush()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_log_file.flush(); // 刷新文件流，确保数据写入磁盘
}

bool Log::isClosed() {
    return m_close_log;
}

Log::Log() : m_running(false), m_level(INFO) {
    // m_thread = std::thread(&Log::logThread, this);  // 启动日志线程
}

Log::~Log()
{
    m_running = false;
    m_cv.notify_all();  // 唤醒线程，退出
    if(m_thread.joinable()){
        m_thread.join();        // 等待日志线程结束
    }
    if(m_log_file.is_open()){
        m_log_file.close();     // 关闭日志文件流
    }

}

void Log::logThread()
{
    while(m_running) {
        std::unique_lock<std::mutex> lock(m_mutex);
        // 等待队列非空或程序退出
        m_cv.wait(lock, [this]() {return !m_log_queue.empty() || !m_running;});

        // 写入日志
        while (!m_log_queue.empty()) {
            std::string log_entry = m_log_queue.front();
            m_log_queue.pop();

            m_log_file << log_entry << std::endl;   // 写入文件
            std::cout << log_entry << std::endl;
        }
    }
}

std::string Log::formatLog(LogLevel level, const std::string &message)
{
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm = *std::localtime(&now);

    std::stringstream log_stream;
    log_stream << std::put_time(&tm, "[%Y-%m-%d %H:%M:%S]");        // 格式化时间
    log_stream << " [" << levelToString(level) << "] " << message;
    return log_stream.str();
}

std::string Log::levelToString(LogLevel level)
{
    switch(level) {
        case DEBUG: return "DEBUG";
        case INFO: return "INFO";
        case WARN: return "WARN";
        case ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}
void Log::shutdown() {
    if (m_close_log || !m_running) return;

    m_running = false;  // 停止日志线程
    m_close_log = true; // 标记日志已关闭
    m_cv.notify_all();  // 唤醒线程

    if (m_thread.joinable()) {
        m_thread.join();    // 等待线程退出
    }

    // 确保缓冲数据写入
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_log_queue.empty()) {
            m_log_file << m_log_queue.front() << std::endl;
            m_log_queue.pop();
        }
    }

    if (m_log_file.is_open()) {
        m_log_file.close();
    }

    std::cout << "[Log] 日志已关闭。" << std::endl;
}