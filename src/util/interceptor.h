#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <mutex>

// 通用拦截器：用于统一控制访问权限、黑名单、白名单等
class Interceptor {
public:
    // 初始化拦截器是否开启
    void init(bool interceptorClose);

    // 获取单例实例
    static Interceptor& getInstance();

    /*
        注册拦截规则（type维度）：
            type：拦截类型（如 "login_required", "url_blacklist"）
            key：拦截内容（如URL、Method）
            whitelist：是否为白名单模式（true：仅允许列中存在的，false：禁止列中存在的）
    */
    void registerTypeRule(const std::string& type, const std::unordered_set<std::string>& keys, bool whitelist = true);

    // 是否该 key 受到指定 type 的拦截控制
    bool shouldIntercept(const std::string& type, const std::string& key);
     // 是否启用指定拦截类型
    bool isTypeEnable(const std::string& type);
    // 拦截器总开关是否关闭
    bool isInterceptorClose();
    // 执行身份认证拦截逻辑
    // bool intercept(const HttpRequest& http_request, const HttpResponse& http_response);

private:
    std::unordered_map<std::string, std::unordered_set<std::string>> m_typeInterceptorMap;  // 拦截类型 -> 规则集合
    std::unordered_map<std::string, bool> m_typeWhiteListMap;           // 每种拦截类型是否为白名单模式
    std::unordered_map<std::string, bool> m_typeInterceptorEnableMap;   // 每种类型是否启用
    bool m_interceptorClose;                                            // 全局是否关闭拦截器
    std::mutex m_mutex;
};