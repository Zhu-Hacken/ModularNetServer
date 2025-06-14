#include "sql_connection_pool.h"
#include "util/utils.h"
#include <stdexcept>

const std::string BASE_TEXT = "[SqlConnPool] ";

// 单例模式获取实例
SqlConnPool& SqlConnPool::getInstance() {
    static SqlConnPool instance;    // 局部静态变量，线程安全
    return instance;
}

// 初始化连接池
void SqlConnPool::init(const std::string& host, int port,
                       const std::string& user, const std::string& pwd,
                       const std::string& db_name, int conn_size) 
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_host = host;
    m_port = port;
    m_user = user;
    m_password = pwd;
    m_db_name = db_name;
    m_max_conn = conn_size;

    for (int i = 0; i < m_max_conn; ++i) {
        MYSQL* conn = mysql_init(nullptr);
        if (!conn) {
            LOG_ERROR(BASE_TEXT + "MySQL init failed.");
            throw std::runtime_error(BASE_TEXT + "MySQL init failed.");
        }

        conn = mysql_real_connect(conn, host.c_str(), user.c_str(), pwd.c_str(), db_name.c_str(), port, nullptr, 0);

        if (!conn) {
            LOG_ERROR(BASE_TEXT + "MySQL connect failed: " + std::string(mysql_error(conn)));
            throw std::runtime_error(BASE_TEXT + "MySQL connect failed.");
        }

        m_conn_queue.push(conn);
        ++m_free_conn;

    }
    LOG_INFO(BASE_TEXT + "Initialized: " + std::to_string(conn_size) + " connections.");
}

// 获取一个连接（阻塞等待）          
MYSQL* SqlConnPool::getConn() {
    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_conn_queue.empty()) {
        m_cond.wait(lock);
    }
    MYSQL* conn = m_conn_queue.front();
    m_conn_queue.pop();
    --m_free_conn;
    return conn;
}

// 释放连接
void SqlConnPool::releaseConn(MYSQL* conn) {
    if(!conn) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_conn_queue.push(conn);
    ++m_free_conn;
    m_cond.notify_one();    // 唤醒等待线程
}

// 销毁连接池
void SqlConnPool::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    while (!m_conn_queue.empty()) {
        MYSQL* conn = m_conn_queue.front();
        m_conn_queue.pop();
        mysql_close(conn);
    }
    m_free_conn = 0;
    m_max_conn = 0;
    LOG_INFO(BASE_TEXT + "数据库连接池已关闭。");
}

// 获取当前可用连接数
int SqlConnPool::getFreeConnCount() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_free_conn;
}

SqlConnPool::~SqlConnPool() {
    shutdown();
}