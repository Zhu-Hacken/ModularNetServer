#include "redis_client.h"
#include <iostream>
#include "util/utils.h"

const std::string BASE_TEXT = "[RedisClient] ";

RedisClient::RedisClient(const std::string& host, int port) : m_ctx(nullptr), m_connected(false) {
    m_ctx = redisConnect(host.c_str(), port);
    if (m_ctx == nullptr || m_ctx->err) {
        if (m_ctx) {
            LOG_ERROR(BASE_TEXT + "Connection error: " + m_ctx->errstr);
            redisFree(m_ctx);
            m_ctx = nullptr;
        }
        else {
            LOG_ERROR(BASE_TEXT + "Connection error: can not allocate redis context." );
        }
        return;
    }

    m_connected = true;
}
RedisClient::~RedisClient() {
    if (m_ctx) {
        redisFree(m_ctx);
        m_ctx = nullptr;
        m_connected = false;
    }
}

bool RedisClient::set(const std::string& key, const std::string& value) {
    if (!m_connected || !m_ctx) return false;

    redisReply* reply = static_cast<redisReply*>(
        redisCommand(m_ctx, "SET %s %s", key.c_str(), value.c_str())
    );

    if (!reply) {
        LOG_ERROR(BASE_TEXT + "SET command failed: null reply.");
        return false;
    }

    bool success = (reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK");
    if (!success) {
        LOG_ERROR(BASE_TEXT + "SET command error: reply type = " + std::to_string(reply->type));
    }

    freeReplyObject(reply);
    return success;
}

bool RedisClient::get(const std::string& key, std::string& value) {
    if (!m_connected || !m_ctx) return false;

    redisReply* reply = static_cast<redisReply*>(
        redisCommand(m_ctx, "GET %s", key.c_str())
    );

    if (!reply) {
        LOG_ERROR(BASE_TEXT + "GET command failed: null reply.");
        return false;
    }

    bool success = false;

    if (reply->type == REDIS_REPLY_STRING) {
        value = reply->str;
        success = true;
    }
    else if (reply->type == REDIS_REPLY_NIL) {
        // key不存在
        value.clear();
        success = false;
    } else {
        LOG_ERROR(BASE_TEXT + "GET command unexpected reply type: " + std::to_string(reply->type));
        success = false;
    }

    freeReplyObject(reply);
    return success;
}

bool RedisClient::del(const std::string& key) {
    if (!m_connected || !m_ctx) return false;

    redisReply* reply = static_cast<redisReply*>(
        redisCommand(m_ctx, "DEL %s", key.c_str())
    );

    if (!reply) {
        LOG_ERROR(BASE_TEXT + "DEL command failed: null reply.");
        return false;
    }

    bool success = false;

    if (reply->type == REDIS_REPLY_INTEGER) {
        success = (reply->integer > 0); // 删除成功（1）或key不存在（0）
    }
    else{
        LOG_ERROR(BASE_TEXT + "DEL command unexpected reply type: " + std::to_string(reply->type));
    }
    freeReplyObject(reply);
    return success;
}

bool RedisClient::exists(const std::string& key) {
    if (!m_connected || !m_ctx) return false;

    redisReply* reply = static_cast<redisReply*>(
        redisCommand(m_ctx, "EXISTS %s", key.c_str())
    );

    if (!reply) {
        LOG_ERROR(BASE_TEXT + "EXISTS command failed: null reply.");
        return false;
    }

    bool found = false;

    if (reply->type == REDIS_REPLY_INTEGER) {
        found = (reply->integer == 1);  // 存在（1），不存在（0）
    }
    else {
        LOG_ERROR(BASE_TEXT + "EXISTS command unexpected reply type: " + std::to_string(reply->type));
    }

    freeReplyObject(reply);
    return found;
}

bool RedisClient::isConnected() const {
    return m_connected;
}