#include "cache/cache_adapter.h"
#include <iostream>

int main() {
    using std::cout;
    using std::endl;
    const std::string key = "name";
    const std::string value = "Alice";

    // 1️⃣ 初始化为 LRU + Redis 联合模式
    CacheAdapter::getInstance().init(CacheMode::LRU_REDIS, 100);
    cout << "[Init] CacheMode::LRU_REDIS\n";

    // 2️⃣ 测试 set
    CacheAdapter::getInstance().set(key, value);
    cout << "[Set] key = " << key << ", value = " << value << endl;

    // 3️⃣ 测试 get（第一次应从 Redis 命中，同时写入 LRU）
    std::string val;
    if (CacheAdapter::getInstance().get(key, val)) {
        cout << "[Get1] success: " << val << endl;
    } else {
        cout << "[Get1] failed\n";
    }

    // 4️⃣ 测试 contains（应命中）
    bool exists = CacheAdapter::getInstance().contains(key);
    cout << "[Contains] " << (exists ? "exists" : "not found") << endl;

    // 5️⃣ 测试 erase
    CacheAdapter::getInstance().erase(key);
    cout << "[Erase] key = " << key << endl;

    // 6️⃣ 测试 get 失败情况（已被删）
    if (CacheAdapter::getInstance().get(key, val)) {
        cout << "[Get2] still exists after erase: " << val << endl;
    } else {
        cout << "[Get2] correctly not found after erase\n";
    }

    return 0;
}