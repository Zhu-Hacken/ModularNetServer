#pragma once
#include "lru_cache.h"
#include "redis_client.h"
#include <string>
#include <memory>

enum class CacheMode {
    LRU_ONLY,
    REDIS_ONLY,
    LRU_REDIS
};

class CacheAdapter {
public:
    // 单例模式
    static CacheAdapter& getInstance();

    // 初始化模式
    void init(CacheMode mode, size_t lru_capacity = 1000);

    // 基础操作
    bool get(const std::string& key, std::string& value_out);
    void set(const std::string& key, const std::string& value);
    void erase(const std::string& key);
    bool contains(const std::string& key);

    void clear();

private:
    CacheAdapter();
    CacheAdapter(const CacheAdapter&) = delete;
    CacheAdapter& operator=(const CacheAdapter&) = delete;

private:
    CacheMode m_mode;
    std::unique_ptr<LRUCache<std::string, std::string>> m_lru;    
    std::unique_ptr<RedisClient> m_redis;
};