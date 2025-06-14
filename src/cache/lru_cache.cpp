// #include "lru_cache.h"

// template<typename K, typename V>
// LRUCache<K,V>::LRUCache(size_t capacity): m_capacity(capacity) {

// }

// template<typename K, typename V>    
// void LRUCache<K,V>::put(const K& key, const V& value) {
//     if (m_capacity == 0) return;
//     std::lock_guard<std::mutex> lock(m_mutex);

//     auto it = m_index.find(key);
//     if (it != m_index.end()) {
//         // 已存在：更新值 + 移动到链表头部
//         it->second->second = value;
//         m_items.splice(m_items.begin(), m_items, it->second);
//         return;
//     }

//     // 不存在：则检查容量
//     if (m_items.size() >= m_capacity) {
//         // 删除尾部元素
//         auto last = m_items.back();
//         m_index.erase(last.first);  // 从 map 移除
//         m_items.pop_back();         // 从 list 移除
//     }

//     m_items.emplace_front(key, value);
//     m_index.emplace(key, m_items.begin());
// }

// template<typename K, typename V>
// bool LRUCache<K,V>::get(const K& key, V& value_out) {
//     std::lock_guard<std::mutex> lock(m_mutex);

//     auto it = m_index.find(key);
//     if (it == m_index.end()) {
//         return false;    // 未命中
//     }

//     // 命中：将该节点移动到链表头部
//     m_items.splice(m_items.begin(), m_items, it->second);

//     // 更新输出值
//     value_out = it->second->second;
//     return true;
// }

// template<typename K, typename V>
// bool LRUCache<K,V>::contains(const K& key) {
//     std::lock_guard<std::mutex> lock(m_mutex);
//     return m_index.find(key) != m_index.end();
// }

// template<typename K, typename V>
// void LRUCache<K,V>::erase(const K& key) {
//     std::lock_guard<std::mutex> lock(m_mutex);
    
//     auto it = m_index.find(key);
//     if (it == m_index.end()) return;

//     // 从list中删除节点
//     m_items.erase(it->second);
//     // 从map中删除映射
//     m_index.erase(it);
// }

// template<typename K, typename V>
// void LRUCache<K,V>::clear() {
//     std::lock_guard<std::mutex> lock(m_mutex);
//     m_items.clear();
//     m_index.clear();
// }

// template<typename K, typename V>
// size_t LRUCache<K,V>::size() const {
//     std::lock_guard<std::mutex> lock(m_mutex);
//     return m_items.size();
// }