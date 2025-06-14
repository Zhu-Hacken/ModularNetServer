#include "interceptor.h"
#include "util/utils.h"
// std::unordered_set<std::string> Interceptor::kAuthWhitelist = {
//     "/login.html",
//     "/register",
//     "/index.html",
//     "/",
//     "/logo.jpg",
//     "/api/hello",
//     "/login",
//     "/welcome.html",
//     "/video.mp4"

// };

const std::string BASE_TEXT = "[Interceptor] "; 

// 初始化拦截器是否开启
void Interceptor::init(bool interceptorClose) {
    m_interceptorClose = interceptorClose;
    m_typeInterceptorMap.clear();
    m_typeInterceptorEnableMap.clear();
    m_typeWhiteListMap.clear();
}

// 获取单例实例
Interceptor& Interceptor::getInstance() {
    static Interceptor instance;
    return instance;
}

/*
    注册拦截规则（type维度）：
        type：拦截类型（如 "login_required", "url_blacklist"）
        key：拦截内容（如URL、Method）
        whitelist：是否为白名单模式（true：仅允许列中存在的，false：禁止列中存在的）
*/
void Interceptor::registerTypeRule(const std::string& type, const std::unordered_set<std::string>& keys, bool whitelist) {
    // 如果拦截器已关闭，则不拦截
    if (isInterceptorClose()) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    m_typeInterceptorMap.insert({type, keys});
    m_typeWhiteListMap[type] = whitelist;
    m_typeInterceptorEnableMap[type] = true;
}

// 是否该 key 受到指定 type 的拦截控制
bool Interceptor::shouldIntercept(const std::string& type, const std::string& key) {
    // 如果拦截器已关闭，则不拦截
    if (isInterceptorClose()) {
        LOG_INFO(BASE_TEXT + "拦截器关闭，直接放行。type = " + type + ", key = " + key); 
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // 类型不存在或不启用
    auto it_enable = m_typeInterceptorEnableMap.find(type);
    if ( !(it_enable != m_typeInterceptorEnableMap.end()
            && it_enable->second)) {
    // if (!isTypeEnable(type)) {   // 不再调用 isTypeEnable 避免重复加锁导致死锁
        LOG_INFO(BASE_TEXT + "拦截类型未启用，放行。type = " + type + ", key = " + key);
        return false;
    }

    // 判断是否需要拦截该type
    auto it_type = m_typeInterceptorMap.find(type);
    if (it_type == m_typeInterceptorMap.end()) {
        LOG_INFO(BASE_TEXT + "未找到类型对应规则，放行。type = " + type + ", key = " + key); 
        return false;
    }
    
    
    // 判断该key是否在type的规则列表中
    bool keyExists = it_type->second.count(key);
    // 白名单：命中则不拦截，否则拦截
    // 黑名单：命中则拦截，否则不拦截
    bool isWhitelist = true;


    auto it_white = m_typeWhiteListMap.find(type);

    if (it_white != m_typeWhiteListMap.end()) {
        isWhitelist = it_white->second;
    }

    bool res = isWhitelist ? !keyExists : keyExists;

    LOG_INFO(BASE_TEXT + 
        "拦截判断完成: type = " + type + 
        ", key = " + key + 
        ", isWhitelist = " + (isWhitelist ? "true" : "false") +
        ", keyExists = " + (keyExists ? "true" : "false") +
        ", shouldIntercept = " + (res ? "true" : "false"));

    // 白名单：命中放行，未命中拦截；黑名单：命中拦截，未命中放行
    return res;       
    
}

// 是否启用指定拦截类型
bool Interceptor::isTypeEnable(const std::string& type) {
    // 如果拦截器已关闭，则不管
    if (isInterceptorClose()) return false;

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_typeInterceptorEnableMap.find(type);
    return ( it != m_typeInterceptorEnableMap.end()) 
            && it->second; 
}



// 拦截器总开关是否关闭
bool Interceptor::isInterceptorClose() {
    return m_interceptorClose;
}