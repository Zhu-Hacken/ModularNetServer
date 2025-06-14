#include "config_manager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include "third_party/nlohmann/json.hpp"
#include "util/utils.h"


using Json = nlohmann::json;

const std::string& BASE_TEXT = "[ConfigManager] ";


ConfigManager::ConfigManager() {
    m_config_path = SysUtils::getRootPath() + "/src/config/server_config.json";
}

ConfigManager::~ConfigManager() {
    m_running = false;
    if (m_watch_thread.joinable()) {
        m_watch_thread.join();
    }
}

ConfigManager& ConfigManager::getInstace() {
    static ConfigManager instance;
    return instance;
}

void ConfigManager::init(bool enable_hot_reload, std::string path) {
    if (path != "") m_config_path = path;
    
    m_config_manager_close = !enable_hot_reload;

    // 初始加载一次配置
    if (!loadFromFile(m_config_path)) {
        LOG_WARN(BASE_TEXT + "First config load failed or no changes.");
    }

    if ( !m_config_manager_close) startWatchThread();    // 内部会自己设置 m_running = true;
}

bool ConfigManager::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_ERROR(BASE_TEXT + "Failed to open config file: " + path);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    std::vector<std::function<void()>> callbacks_to_call;
    {

        std::lock_guard<std::mutex> lock(m_mutex);

        // 如果内容未变，则不更新
        if (content == m_last_loaded_content) {
            return false;
        }

        Json j;
        try {
            j = Json::parse(content);
        } catch (std::exception& e) {
            LOG_ERROR(BASE_TEXT + "Failed to parse config JSON: " + e.what());
            return false;
        }


        // 是否为首次加载（初始化）
        bool first_load = m_config.empty();
        std::vector<std::string> changed_keys;

        for (Json::iterator it = j.begin(); it != j.end(); ++it) {
            const std::string& key = it.key();
            std::string new_val = it.value().dump(); 
            
            bool is_changed = m_config.count(key) == 0 || m_config[key] != new_val;
            bool is_hot_reloadable = m_hot_reloadable_keys.count(key) > 0;

            // 首次运行加载所有值
            // 后续热更新加载“热更新”值
            if (first_load || is_hot_reloadable) {
                if (is_changed) {
                    m_config[key] = new_val;
                    if (!first_load && is_hot_reloadable) {
                        changed_keys.push_back(key);
                        LOG_INFO(BASE_TEXT + "Config updated: " + key + " = " + new_val);
                    }
                }
            }
            else {
                LOG_DEBUG(BASE_TEXT + "Ignored config key: " + key);
            }
        }

        m_last_loaded_content = content;

        // 首次加载不执行回调（由各个模块自行初始化）
        if (!first_load) {
            for (const auto& key: changed_keys) {
                auto cb_it = m_key_callbacks.find(key);
                if (cb_it != m_key_callbacks.end()) {
                    callbacks_to_call.push_back(cb_it->second); // 暂存回调
                    // cb_it->second();
                }
            }
        }
    }
    
    // 执行回调
    for (const auto& cb: callbacks_to_call) {
        cb();
    }

    return true;
}

void ConfigManager::startWatchThread() {
    bool expected = false;
    if (!m_running.compare_exchange_strong(expected, true)) return;   // 已在运行

    m_watch_thread = std::thread([this]() {
        LOG_INFO(BASE_TEXT + "Hot reload thread started, watching: " + m_config_path);
        while (m_running.load()) {
            const int interval = 10;
            std::this_thread::sleep_for(std::chrono::seconds(interval));
            loadFromFile(m_config_path);
        }
        LOG_INFO(BASE_TEXT + "Hot reload thread exited.");
    });
}

// 获取指定 key 对应的值（字符串）
std::string ConfigManager::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_config.find(key);
    return (it != m_config.end() ? it->second : "");
}

int ConfigManager::getInt(const std::string& key, int defaultVal) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_config.find(key);

    if (it == m_config.end()) return defaultVal;

    try {
        return std::stoi(it->second);
    } catch (...) {
        return defaultVal;
    }

}

bool ConfigManager::getBool(const std::string& key, bool defaultVal) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_config.find(key);
    if (it == m_config.end()) return defaultVal;

    std::string val = it->second;
    return val == "1" || val == "true" || val == "True";
}

std::string& ConfigManager::getPath() {
    return m_config_path;
}

// 注册配置变更后的回调函数
void ConfigManager::registerCallback( std::string key, CallBack cb){    // 注册配置变动回调
    m_key_callbacks.insert({key, cb});
}