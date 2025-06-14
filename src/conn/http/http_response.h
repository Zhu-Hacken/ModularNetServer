#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include "http_base.h"

// #include "third_party/nlohmann/json.hpp"
 
// using Json = nlohmann::json;

class HttpResponse {
public:
    void init(std::shared_ptr<HttpBase> http_base);
    bool isBuilt(); // 返回响应内容是否已经构建

    // === Setter ===
    void setStatus(int code, const std::string& status_text);
    void setHeader(const std::string& key, const std::string& value);
    void setCookie(const std::string& key, const std::string& value);
    void setBody(const std::string& body);
    void setContentType(const std::string& content_type);
    void setKeepAlive(bool keep_alive);
    void sendRedirect(const std::string& new_url);
    void setNewUrl(const std::string& new_url);

    // === Getter
    const std::string& getUrl() const;


    // === 发送响应信息 ===
    void sendText(int status_code, const std::string& title, const std::string& message);
    void sendJson(int status_code, const std::string& json_str);
    void sendJson(int status_code, const Json& data);
    void sendRESTfulJson(int status_code, const std::string& message, const std::string& data = "");
    void sendRESTfulJson(int status_code, const std::string& message, const Json& data);
private:

    // ===  ===

    // void setCookie(const std::string& key, const std::string& value);
    // void setUrl(const std::string& url) {m_url = url;}

    // 根据解析完成的信息，构造响应
    // bool generateResponse();  

    friend class HttpConn;
    // 构建RESTful Json
    Json buildRESTfulJson(int status_code, const std::string& message, const std::string& data);
    Json buildRESTfulJson(int status_code, const std::string& message, const Json& data);

    // 构造响应头
    int buildResponseHeader(int status_code, const std::string& status_text,
                            const std::string& content_type, int content_length,
                            const std::unordered_map<std::string, std::string>& extra_headers = {});
    // 获取最终响应数据（供HttpConn调用 write发送）
    const std::string& getResponseData() const; 

private:
    std::shared_ptr<HttpBase> m_http_base;  // 保存基本的http属性
    int m_status_code;              // 状态码
    std::string m_status_text;      // 状态文本
    std::string m_response_body;         // 响应体
    std::string m_response_url;                // 新url
    std::unordered_map<std::string, std::string> m_response_headers; // 响应头（如Cookie，Content-Type）
    std::unordered_map<std::string, std::string> m_response_cookies;    // 保存响应cookie
    std::string m_response_data;    // HTTP响应
};