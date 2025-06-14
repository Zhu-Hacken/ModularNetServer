#include "sql_transaction.h"
#include "util/utils.h"
#include <ostream>

const std::string BASE_TEXT = "[SqlTransaction] ";

SqlTransaction::SqlTransaction(MYSQL* conn):m_conn(conn), m_committed(false) {
    begin();
}
SqlTransaction::~SqlTransaction() {
    rollback();
}

void SqlTransaction::begin() {
    if (mysql_query(m_conn, "START TRANSACTION")) {
        std::ostringstream oss;
        oss << "MySQL START TRANSACTION failed: " << mysql_error(m_conn);
        LOG_ERROR( BASE_TEXT + oss.str().c_str());
        throw std::runtime_error("START TRANSACTION failed");
    }
    LOG_DEBUG( BASE_TEXT + "Transaction started.");

}

void SqlTransaction::commit() {
    if (mysql_commit(m_conn)) {
        std::ostringstream oss;
        oss << "MySQL COMMIT failed: " << mysql_error(m_conn);
        LOG_ERROR( BASE_TEXT + oss.str().c_str());
        throw std::runtime_error("COMMIT failed");
    }
    m_committed = true;
    LOG_DEBUG( BASE_TEXT + "Transaction committed.");
}

void SqlTransaction::rollback() noexcept {
    if (!m_committed) {
        if (mysql_rollback(m_conn) != 0) {
            try {
                std::ostringstream oss;
                oss << "MySQL rollback failed: " << mysql_error(m_conn);
                LOG_ERROR( BASE_TEXT + oss.str().c_str());
            } catch (...) {
                // 被析构函数调用时，不允许出现异常
            }
        } else {
            LOG_INFO( BASE_TEXT + "MySQL rollback executed.");
        }
    }
}