#include "token_manager.h"
#include "util/utils.h"
#include <chrono>
#include <sstream>
#include "log/logs.h"

int TokenManager::timer_seed = 1000000;

const std::string secret_key = "my_server_secret_key";
const std::string BASE_TEXT = "[TokenManager] ";

TokenManager& TokenManager::getInstance() {
    static TokenManager instance;
    return instance;
}

Token TokenManager::createToken(std::string username, int timeout_ms) {
    // token初始示例：token_1687777777123456_thread42_user_zhangsan
    // token最终实例：base64.signature

    std::lock_guard<std::mutex> lock(m_mutex);

    // 获取时间戳
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    // 获取线程id
    auto tid = SysUtils::getThreadIdStr();

    std::ostringstream oss;
    oss << "token_" << now << "_" << tid << "_" << username;
    std::string token_raw = oss.str();
    std::string token_base64 = CryptoUtils::base64Encode(token_raw);
    std::string token_signature = CryptoUtils::hmacSha256(token_base64, secret_key);
    
    Token token = token_base64 + "." + token_signature;

    m_token_username[token] = username;
    LOG_INFO(BASE_TEXT + " createToken: " + token);
    int timer_id = timer_seed;

    m_token_timers[token] = timer_id;
    timer_seed++;
    m_timer_manager.addTimer(timer_id, [this, token]() {
        this->cleanToken(token);
    }, timeout_ms);

    return token;
}

bool TokenManager::isTokenValid(const Token& token) {
    // 先判断格式是否正确
    size_t pos = token.find('.');
    if (pos == std::string::npos) return false;
    
    // 提取base64与signature
    std::string token_base64 = token.substr(0, pos);
    std::string token_signature = token.substr(pos + 1);
    // 判断签名是否正确
    if (CryptoUtils::hmacSha256(token_base64, secret_key) != token_signature) return false;
    
    // 判断签名是否存在我们的映射表中
    if (!exist(token)) return false;
    return true;
}

bool TokenManager::isTokenExpired(const Token& token) {
    // 判断是否过期
    return m_timer_manager.isExpired(m_token_timers[token]);
}

void TokenManager::removeToken(const Token& token) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 先判断是否有效
    if (!isTokenValid(token)) return;

    auto it_username = m_token_username.find(token);
    if (it_username != m_token_username.end()) {
        m_token_username.erase(token);
    }

    auto it_timer = m_token_timers.find(token);
    if (it_timer != m_token_timers.end()) {
        m_token_timers.erase(it_timer);
    }


}

std::string TokenManager::getUsername(const Token& token) {
    if (!isTokenValid(token)) return "";
    if (isTokenExpired(token)) return "";
    auto it = m_token_username.find(token);
    if (it == m_token_username.end()) return "";
    return it->second;
}

void TokenManager::refreshToken(const Token& token, int timeout_ms) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!isTokenValid(token)) return;
    if (isTokenExpired(token)) return;

    auto it_timer = m_token_timers.find(token);
    if (it_timer == m_token_timers.end()) return;

    int timer_id = it_timer->second;
    m_timer_manager.addTimer(timer_id, [this, token]() {
        this->cleanToken(token);
    }, timeout_ms);
    LOG_INFO(BASE_TEXT + "续期 token = " + token);
}

bool TokenManager::exist(const Token& token) {
    return m_token_username.find(token) != m_token_username.end();
}

void TokenManager::cleanToken(const Token& token) {
    removeToken(token);
    LOG_INFO(BASE_TEXT + "token expired and cleaned: " + token);
}