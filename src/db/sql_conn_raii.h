#pragma once
#include <mysql/mysql.h>

/*
 * SqlConnRAII：用于自动管理 MySQL 数据库连接，遵循 RAII 思想
 * 作用：构造时自动从连接池中获取连接，析构时自动归还连接，避免泄漏
 */
class SqlConnRAII {
public:
    // 构造函数：从连接池中获取连接
    SqlConnRAII(MYSQL** conn);

    // 析构函数：释放连接归还连接池
    ~SqlConnRAII();

private:
    MYSQL* m_conn;      // 当前持有的MySQL连接
};