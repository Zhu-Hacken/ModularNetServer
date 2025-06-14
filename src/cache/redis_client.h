#pragma once

#include <string>
#include <hiredis/hiredis.h>

class RedisClient {
public:
    RedisClient(const std::string& host = "127.0.0.1", int port = 6379);
    ~RedisClient();

    bool set(const std::string& key, const std::string& value);
    bool get(const std::string& key, std::string& value);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    bool isConnected() const;

    RedisClient(const RedisClient&) = delete;
    RedisClient& operator=(const RedisClient&) = delete;


private:
    redisContext* m_ctx;
    bool m_connected;
};