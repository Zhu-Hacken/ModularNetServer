#pragma once
#include <string>
#include <mysql/mysql.h>
#include <queue>
#include <mutex>
#include <condition_variable>

class SqlConnPool {
public:
    // 单例模式获取实例
    static SqlConnPool& getInstance();

    // 初始化连接池
    void init(const std::string& host, int port,
              const std::string& user, const std::string& pwd,
              const std::string& db_name, int conn_size);

    // 获取一个连接（阻塞等待）          
    MYSQL* getConn();

    // 释放连接
    void releaseConn(MYSQL* conn);

    // 销毁连接池
    void shutdown();

    // 获取当前可用连接数
    int getFreeConnCount();

private:
    SqlConnPool() = default;
    ~SqlConnPool();

    // 禁用拷贝构造和赋值
    SqlConnPool(const SqlConnPool&) = delete;
    SqlConnPool operator=(const SqlConnPool&) = delete;

private:
    std::queue<MYSQL*> m_conn_queue;    // 连接队列
    std::mutex m_mutex;                 // 互斥锁
    std::condition_variable m_cond;     // 条件变量（阻塞等待）

    int m_max_conn = 0;                 // 最大连接数
    int m_free_conn = 0;                // 当前空闲连接数

    std::string m_host;
    std::string m_user;
    std::string m_password;
    std::string m_db_name;
    int m_port = 3306;
};