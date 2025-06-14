#include "test_tx_dao.h"
#include "db/sql_conn_raii.h"
#include "db/sql_executor.h"
#include "db/sql_transaction.h"
#include "util/utils.h"

const std::string BASE_TEXT = "[TextTxDao] ";

bool TextTxDao::insertUser(std::string &username, int age) {
    MYSQL* conn = nullptr;
    SqlConnRAII conn_raii(&conn);
    if (!conn) return false;

    SqlTransaction tx(conn);

    SqlExecutor exec(conn);
    const std::string sql = "INSERT INTO test_tx_user (username, age) VALUES (?, ?)";
    if (!exec.prepare(sql)) return false;
    exec.bindParam(0, username);
    exec.bindParam(1,age);
    if(!exec.execute())return false;

    if (username == "rollback") {
        // 模拟业务失败
        LOG_INFO("用户名为 rollback，触发事务回滚。");
        return false;
    }

    tx.commit();
    return true;
}