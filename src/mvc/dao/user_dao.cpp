#include "user_dao.h"
#include "db/sql_conn_raii.h"
#include "db/sql_executor.h"
#include "log/log_utils.h"
#include <mysql/mysql.h>

const std::string BASE_TEXT = "[UserDao] ";

// 校验用户名和密码是否正确
bool UserDao::checkUser(const std::string& username, const std::string& password) {
    MYSQL* conn = nullptr;
    SqlConnRAII conn_raii(&conn);

    if(!conn) {
        LOG_ERROR(BASE_TEXT + "获取数据库连接失败");
        return false;
    }
    // =======================================
    // SqlExecutor executor(conn);

    // if (!executor.prepare("SELECT username FROM user LIMIT 1")) {
    //     LOG_ERROR(BASE_TEXT + "prepare failed");
    //     return false;
    // }

    // if (!executor.execute()) {
    //     LOG_ERROR(BASE_TEXT + "execute failed");
    //     return false;
    // }

    // auto results = executor.fetchAll();
    // LOG_DEBUG(BASE_TEXT + "结果行数 = " + std::to_string(results.size()));
    // return false;
    // =======================================
    // 创建SqlExecutor
    SqlExecutor executor(conn);

    // 预处理sql（防注入）
    const std::string sql = "SELECT password FROM user WHERE username = ? LIMIT 1";
    // const std::string sql = "SELECT password FROM user WHERE username = 'admin' LIMIT 1";
    if (!executor.prepare(sql)) {
        LOG_ERROR(BASE_TEXT + "prepare() failed.");
        return false;
    }

    // 绑定参数
    if (!executor.bindParam(0, username)) {
        LOG_ERROR(BASE_TEXT + "bindParam username failed.");
        return false;
    }
    
    // 执行查询
    if (!executor.execute()) {
        LOG_ERROR(BASE_TEXT + "execute failed.");
        return false;
    }

    // 获取结果
    std::vector<std::map<std::string, std::string>> results = executor.fetchAll();
    if (results.empty()) {
        LOG_INFO(BASE_TEXT + "用户不存在：" + username);
        return false;
    }

    const std::string& stored_pwd = results[0]["password"];
    return stored_pwd == password;

    // std::string query = "SELECT password FROM user WHERE username = '" + username + "' LIMIT 1";
    // if (mysql_query(conn, query.c_str())) {
    //     LOG_ERROR(BASE_TEXT + "查询失败：" + std::string(mysql_error(conn)));
    //     return false;
    // }

    // MYSQL_RES* res = mysql_store_result(conn);
    // if (!res) {
    //     LOG_ERROR(BASE_TEXT + "获取结果失败：" + std::string(mysql_error(conn)));
    //     return false;
    // }

    // MYSQL_ROW row = mysql_fetch_row(res);
    // if (row && row[0]) {
    //     std::string stored_pwd(row[0]);
    //     mysql_free_result(res);
    //     return stored_pwd == password;
    // }

    // mysql_free_result(res);
}

// 插入新用户信息，返回是否成功
bool UserDao::insertUser(const std::string& username, const std::string& password) {
    MYSQL* conn = nullptr;
    SqlConnRAII conn_raii(&conn);

    if (!conn) {
        LOG_ERROR(BASE_TEXT + "获取数据库连接失败");
        return false;
    }

    // 构造插入语句
    std::string query = "INSERT INTO user(username, password) VALUES('" + username +"', '" + password + "')";

    if (mysql_query(conn, query.c_str()) != 0) {
        LOG_ERROR(BASE_TEXT + "插入用户失败：" + std::string(mysql_error(conn)));
        return false;
    }
    return true;
}