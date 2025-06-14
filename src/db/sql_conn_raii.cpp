#include "sql_conn_raii.h"
#include "util/utils.h"
#include "db/sql_connection_pool.h"

// 构造函数：从连接池中获取连接
SqlConnRAII::SqlConnRAII(MYSQL** conn)
    : m_conn(NULL)
{
    if (conn == nullptr) {
        LOG_WARN("[SqlConnRAII] 构造函数传入的MYSQL**为nullptr");
        return;
    }

    m_conn = SqlConnPool::getInstance().getConn();
    *conn = m_conn;
    
}

// 析构函数：释放连接归还连接池
SqlConnRAII::~SqlConnRAII() {
    if (m_conn) {
        SqlConnPool::getInstance().releaseConn(m_conn);
        m_conn = nullptr;
    }
}