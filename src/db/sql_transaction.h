#pragma once

#include <mysql/mysql.h>

// RAII 封装事务对象：支持 begin/commit/rollback
class SqlTransaction{
public:
    explicit SqlTransaction(MYSQL* conn);
    ~SqlTransaction();

    // 提交事务，如果调用后析构将不会 rollback
    void commit();  
    // 回滚事务，或在析构时自动回滚
    void rollback() noexcept;   

    // 禁止拷贝和无参构造
    SqlTransaction() = delete;
    SqlTransaction(const SqlTransaction&) = delete;
    SqlTransaction& operator=(const SqlTransaction&) = delete;

private:
    // 开启一个事务，构造时自动调用
    void begin();   

    MYSQL* m_conn;
    bool m_committed;   // 标志位：是否已成功提交
};