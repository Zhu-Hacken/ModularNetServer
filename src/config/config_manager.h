#pragma once
#include <string>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <vector>
#include <thread>
#include <atomic>

// using CallBack = std::function<void(const std::string& newValue)>;
using CallBack = std::function<void()>;

/*
 * 配置管理器：支持Json配置文件加载，定时检测并热更新
*/
class ConfigManager{
public:


    // 获取单例实例
    static ConfigManager& getInstace();
    // 初始化
    void init(bool enable_hot_reload, std::string path = "");

    // 从指定路径加载配置文件（Json文件，转为键值对）
    bool loadFromFile(const std::string& path);
    // 启动后台线程，定期检测配置文件是否变化，变化时触发回调
    void startWatchThread();    
    // 获取指定 key 对应的值（字符串）
    std::string get(const std::string& key);
    // 获取指定 key 对应的 int 值，支持默认值
    int getInt(const std::string& key, int defaultVal = 0);
    bool getBool(const std::string& key, bool defaultVal = false);
    // 获取配置文件地址
    std::string& getPath();
    // 注册配置变更后的回调函数
    void registerCallback(std::string key, CallBack cb);    // 注册配置变动回调

private:
    ConfigManager();
    ~ConfigManager();


    bool m_config_manager_close;                            // 热更新是否关闭
    std::unordered_map<std::string, std::string> m_config;  // 配置键值对
    std::vector<std::function<void()>> m_callbacks;         // 配置变更回调
    std::mutex m_mutex;                                     // 互斥锁
    std::string m_last_loaded_content;                      // 上次加载的内容
    std::string m_config_path;                              // 配置文件路径
    std::thread m_watch_thread;                             // 后台线程
    std::atomic<bool> m_running;                            // 后台线程是否在运行

    std::unordered_map<std::string, CallBack> m_key_callbacks;  // key -> 回调函数 映射

    const std::unordered_set<std::string> m_hot_reloadable_keys = {
        "log_close",
        "rate_limiter_close",
        "interceptor_close",
        "log_level",
        "cache_mode"
    };
};