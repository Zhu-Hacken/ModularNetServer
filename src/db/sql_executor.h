#pragma once

#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <map>

// SqlExecutor: 封装 MYSQL 预处理执行流程，支持参数绑定、安全查询与更新
class SqlExecutor {
public:
    explicit SqlExecutor(MYSQL* conn);
    SqlExecutor() = delete;

    // 准备SQL语句
    bool prepare(const std::string& sql);

    // 绑定参数（按位置index），支持重载
    bool bindParam(int index, const std::string& value);
    bool bindParam(int index, int value);

    // 执行非查询语句
    bool execute();

    // 查询语句
    std::vector<std::map<std::string, std::string>> fetchAll();

    // 禁止拷贝和无参构造
    ~SqlExecutor();
    SqlExecutor(const SqlExecutor&) = delete;
    SqlExecutor& operator=(const SqlExecutor&) = delete;

private:
    MYSQL* m_conn;
    MYSQL_STMT* m_stmt;
    std::vector<MYSQL_BIND> m_bindParams; 

    // 内存管理
    std::vector<void*>    m_allocatedBuffers;   // 跟踪buffer
    std::vector<unsigned long*> m_allocatedLengths; // 跟踪length
};