#include "sql_executor.h"
#include "util/utils.h"
#include <cstring>

const std::string BASE_TEXT = "[SqlExecutor] ";

SqlExecutor::SqlExecutor(MYSQL* conn) 
    : m_conn(conn), m_stmt(nullptr) {
    m_stmt = mysql_stmt_init(m_conn);
    if (!m_stmt) {
        LOG_ERROR(BASE_TEXT + "mysql_stmt_init failed.");
        throw std::runtime_error("Failed to init statement.");
    }
    LOG_DEBUG(BASE_TEXT + "SqlExecutor statement initialized.");
}

SqlExecutor::~SqlExecutor() {
    if (m_stmt) {
        mysql_stmt_close(m_stmt);
        LOG_DEBUG(BASE_TEXT + "SqlExecutor statement closed.");
    }
    for (void* buf: m_allocatedBuffers) {
        free(buf);
    }
    for (unsigned long* len : m_allocatedLengths) {
        delete len;
    }
}

bool SqlExecutor::prepare(const std::string& sql) {
    if (mysql_stmt_prepare(m_stmt, sql.c_str(), sql.length()) != 0) {
        std::ostringstream oss;
        oss << BASE_TEXT << "mysql_stmt_prepare failed: " << mysql_stmt_error(m_stmt);
        LOG_ERROR(oss.str().c_str());
        return false;
    }
    LOG_DEBUG(BASE_TEXT + "SQL prepared: " + sql);
    return true;
}

bool SqlExecutor::bindParam(int index, const std::string& value) {
    if (index >= static_cast<int>(m_bindParams.size())) {
        m_bindParams.resize(index + 1);
    }

    MYSQL_BIND& param = m_bindParams[index];
    memset(&param, 0, sizeof(MYSQL_BIND));

    char *buf = (char*)malloc(value.size() + 1);
    memcpy(buf, value.c_str(), value.size());
    buf[value.size()] = '\0';

    param.buffer_type = MYSQL_TYPE_STRING;
    param.buffer = buf;
    param.buffer_length = value.size() + 1;
    param.is_null = 0;
    unsigned long* len = new unsigned long(value.size());
    param.length = len;

    m_allocatedBuffers.push_back(buf);
    m_allocatedLengths.push_back(len);

    return true;
}

bool SqlExecutor::bindParam(int index, int value) {
    if (index >= static_cast<int>(m_bindParams.size())) {
        m_bindParams.resize(index + 1);
    }

    MYSQL_BIND& param = m_bindParams[index];
    memset(&param, 0, sizeof(MYSQL_BIND));

    void* buf = malloc(sizeof(int));
    memcpy(buf, &value, sizeof(int));

    param.buffer_type = MYSQL_TYPE_LONG;
    param.buffer = buf;
    param.buffer_length = sizeof(int);
    param.is_null = 0;

    unsigned long* len = new unsigned long(sizeof(int));
    param.length = len;

    m_allocatedBuffers.push_back(buf);
    m_allocatedLengths.push_back(len);
    return true;
}

bool SqlExecutor::execute() {
    // 绑定参数
    if (!m_bindParams.empty()) {
        unsigned long expected = mysql_stmt_param_count(m_stmt);
        if (m_bindParams.size() != expected) {
            std::ostringstream oss;
            oss << BASE_TEXT << "Parameter count mismatch. expected=" << expected
                << ", actual=" << m_bindParams.size();
            LOG_ERROR(oss.str().c_str());
            return false;
        }
        if (mysql_stmt_bind_param(m_stmt, m_bindParams.data()) != 0) {
            std::ostringstream oss;
            oss << BASE_TEXT << "mysql_stmt_bind_param failed: " << mysql_stmt_error(m_stmt);
            LOG_ERROR(oss.str().c_str());
            return false;
        }
        LOG_DEBUG(BASE_TEXT + "All parameters bound. Executing statement...");
    }

    if (mysql_stmt_execute(m_stmt) != 0) {
        std::ostringstream oss;
        oss << BASE_TEXT << "mysql_stmt_execute failed: " << mysql_stmt_error(m_stmt);
        LOG_ERROR(oss.str().c_str());
        return false;
    }

    my_ulonglong rows = mysql_stmt_affected_rows(m_stmt);   // SELECT 时返回结果行数
LOG_DEBUG(BASE_TEXT + "Server says affected_rows = "
          + std::to_string(rows)); 

    LOG_DEBUG(BASE_TEXT + "SQL statement executed successfully.");
    return true;
}

std::vector<std::map<std::string, std::string>> SqlExecutor::fetchAll() {
    std::vector<std::map<std::string, std::string>> results;

    if (mysql_stmt_store_result(m_stmt) != 0) {
        std::ostringstream oss;
        oss << "mysql_stmt_store_result failed:" << mysql_stmt_error(m_stmt);
        LOG_ERROR(BASE_TEXT + oss.str().c_str());
        return results;
    }

    // store_result 成功后立刻打印
my_ulonglong total = mysql_stmt_num_rows(m_stmt);
LOG_DEBUG(BASE_TEXT + "stmt_num_rows = " + std::to_string(total));

    // 获取结果集元信息（字段名等）
    MYSQL_RES* meta = mysql_stmt_result_metadata(m_stmt);
    if (!meta) {
        LOG_ERROR(BASE_TEXT + "Failed to retrieve result metadata.");
        return results;
    }

    int column_count = mysql_num_fields(meta);
    std::vector<MYSQL_BIND> result_binds(column_count);
    std::vector<uint8_t> is_null(column_count);             // 是否为空
    std::vector<unsigned long> lengths(column_count);   // 数据实际长度
    std::vector<std::string> column_names;

    MYSQL_FIELD* fields = mysql_fetch_fields(meta);
    for (int i = 0; i < column_count; ++i) { 
        column_names.push_back(fields[i].name);
        result_binds[i].buffer_type = MYSQL_TYPE_STRING;
        char* buf = (char*)malloc(1024);
        result_binds[i].buffer = buf;  // 每列最大1024字节
        m_allocatedBuffers.push_back(buf);
        result_binds[i].buffer_length = 1024;
        result_binds[i].is_null = reinterpret_cast<bool*>(&is_null[i]);
        result_binds[i].length = &lengths[i];
        result_binds[i].error = nullptr;
    }

    if (mysql_stmt_bind_result(m_stmt, result_binds.data()) != 0) {
        LOG_ERROR(BASE_TEXT + "mysql_stmt_bind_result failed.");
        mysql_free_result(meta);
        return results;
    }

    // int code = mysql_stmt_fetch(m_stmt);
    // LOG_DEBUG(BASE_TEXT + "first fetch code = " + std::to_string(code));
    int fetch_code;
    // 开始fetch数据
    while ((fetch_code = mysql_stmt_fetch(m_stmt)) != MYSQL_NO_DATA) {
        if(fetch_code == 1) {   // 错误
            LOG_ERROR(BASE_TEXT + "fetch error: " + mysql_stmt_error(m_stmt));
            break;
        }
        std::map<std::string, std::string> row;
        
        for (int i = 0; i < column_count; ++i) {
            if (is_null[i]) {
                row[column_names[i]] = "NULL";
            } else {
                row[column_names[i]] = std::string(static_cast<char*>(result_binds[i].buffer), lengths[i]);
            }
        }

        results.push_back(std::move(row));
    }
    LOG_DEBUG(BASE_TEXT + "Result row count = " + std::to_string(results.size()));

    mysql_free_result(meta);
    LOG_DEBUG(BASE_TEXT + "SQL SELECT fetchAll() completed.");

    return results;
}