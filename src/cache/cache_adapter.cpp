#include "cache_adapter.h"
#include "util/utils.h"

const std::string BASE_TEXT = "[CacheAdapter] ";

CacheAdapter::CacheAdapter() {}

CacheAdapter& CacheAdapter::getInstance() {
    // 单例模式
    static CacheAdapter instance;
    return instance;
}

void CacheAdapter::init(CacheMode mode, size_t lru_capacity) {
    m_mode = mode;

    if (mode == CacheMode::LRU_ONLY || mode == CacheMode::LRU_REDIS) {
        m_lru = std::unique_ptr<LRUCache<std::string, std::string>>(
            new LRUCache<std::string, std::string>(lru_capacity)
        );
        LOG_INFO(BASE_TEXT + "LRU initialized. Capacity = " + std::to_string(lru_capacity));
    }
    if (mode == CacheMode::REDIS_ONLY || mode == CacheMode::LRU_REDIS) {
        m_redis = std::unique_ptr<RedisClient>(
            new RedisClient()
        );
        if (!m_redis->isConnected()) {
            LOG_WARN(BASE_TEXT + "Redis connection failed during init.");
        }
        else {
            LOG_INFO(BASE_TEXT + "Redis initialized and connected.");
        }
        
    }
}

// 基础操作
bool CacheAdapter::get(const std::string& key, std::string& value_out) {
    if ((m_mode == CacheMode::LRU_ONLY || m_mode == CacheMode::LRU_REDIS) && m_lru) {
        if (m_lru->get(key, value_out)) {
            LOG_DEBUG(BASE_TEXT + "[Hit] LRU key = " + key);
            return true;
        }
    }

    if ((m_mode == CacheMode::REDIS_ONLY || m_mode == CacheMode::LRU_REDIS) && m_redis) {
        if (m_redis->get(key, value_out)) {
            LOG_DEBUG(BASE_TEXT + "[Hit] Redis key = " + key);
            // 如果是LRU_REDIS模式，Redis命中后写入LRU
            if (m_mode == CacheMode::LRU_REDIS && m_lru) {
                m_lru->put(key, value_out);
                LOG_DEBUG(BASE_TEXT + "[Backfill] Redis → LRU key = " + key);
            }
            return true;
        } else {
            LOG_DEBUG(BASE_TEXT + "[Miss] Redis key = " + key);
        }
    }
    LOG_DEBUG(BASE_TEXT + "[Miss] All caches key = " + key);
    return false;
}

void CacheAdapter::set(const std::string& key, const std::string& value) {
    if ((m_mode == CacheMode::LRU_ONLY || m_mode == CacheMode::LRU_REDIS) && m_lru) {
         m_lru->put(key, value);
         LOG_DEBUG(BASE_TEXT + "LRU set key = " + key);
    }
    if ((m_mode == CacheMode::REDIS_ONLY || m_mode == CacheMode::LRU_REDIS) && m_redis) {
        if ( !m_redis->set(key, value)) {
            LOG_WARN(BASE_TEXT + "Redis set failed for key = " + key);
        }
        else {
            LOG_DEBUG(BASE_TEXT + "Redis set key = " + key);
        }
    }
}
void CacheAdapter::erase(const std::string& key) {
    if ((m_mode == CacheMode::LRU_ONLY || m_mode == CacheMode::LRU_REDIS) && m_lru) {
        m_lru->erase(key);
        LOG_DEBUG(BASE_TEXT + "LRU erase key = " + key);
    }
    if ((m_mode == CacheMode::REDIS_ONLY || m_mode == CacheMode::LRU_REDIS) && m_redis) {
        m_redis->del(key);
        LOG_DEBUG(BASE_TEXT + "Redis del key = " + key);
    }
}
bool CacheAdapter::contains(const std::string& key) {
    if ((m_mode == CacheMode::LRU_ONLY || m_mode == CacheMode::LRU_REDIS) && m_lru) {
        if(m_lru->contains(key)) {
            LOG_DEBUG(BASE_TEXT + "LRU contains key = " + key);
            return true;
        }
    }
    if ((m_mode == CacheMode::REDIS_ONLY || m_mode == CacheMode::LRU_REDIS) && m_redis) {
        if (m_redis->exists(key)) {
            LOG_DEBUG(BASE_TEXT + "Redis exists key = " + key);
            return true;
        }
    }

    return false;
}

void CacheAdapter::clear() {
    if (m_lru) {
        m_lru->clear();
        LOG_DEBUG(BASE_TEXT + "LRU cleared.");
    }
    // Redis 暂不清空
}


