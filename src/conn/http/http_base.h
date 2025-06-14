#pragma once

#include <string>
#include <unordered_map>
#include "third_party/nlohmann/json.hpp"
 
using Json = nlohmann::json;
struct ByteRange {
    int start;
    int end;
    bool isValid;
};

class HttpBase {
public:
    enum HttpMethod {
        METHOD_GET,
        METHOD_POST,
        METHOD_HEAD,
        METHOD_PUT,
        METHOD_DELETE,
        METHOD_UNKNOWN
    };

public:
    HttpBase() = default;
    ~HttpBase() = default;

    void init(std::string ip);

    // Getter
    const std::string& getIp() const;
    const std::string& getMethod() const; 
    const std::string& getUrl() const;
    const std::string& getVersion() const;
    const std::string& getBody() const;
    const Json& getJson() const;
    bool getKeepAlive() const;
    const std::string& getConnection() const;
    int getContentLength() const;
    const std::unordered_map<std::string, std::string>& getHeaders() const;
    const std::unordered_map<std::string, std::string>& getFormData() const;
    const std::string& getUsername() const;
    bool isAuthenticated() const;
    const ByteRange getRange() const;

    // Setter
    void setIp(const std::string& ip);
    void setMethod(const std::string& method);
    void setUrl(const std::string& url);
    void setVersion(const std::string& version);
    void setBody(const std::string& body);
    void setJson(const Json&& json);
    void setKeepAlive(bool keep_alive);
    void setConnection(const std::string& connection);
    void setContentLength(int content_length);
    void setHeader(const std::string& key, const std::string& value);
    void setFormData(std::unordered_map<std::string, std::string>&& form_data);
    void setUsername(const std::string& username);


    // 便捷访问
    std::string getHeader(const std::string& key) const;  
    std::string getRequestCookie(const std::string& key) const;
    std::string getParam(const std::string& key) const;
    void parseRange();
    
private:
    std::string m_ip;           // ip地址
    std::string m_method;       // 请求方法，如GET、POST
    std::string m_url;          // 请求路径
    std::string m_version;      // 协议版本，如HTTP/1.1
    std::string m_request_body;         // 请求体（POST表单内容）
    Json m_request_json;                // 请求中的json内容
    bool m_keep_alive;          // 是否保持连接
    std::string m_connection;   // 连接是否关闭
    int m_content_length;       // Content-Length 的值（正文长度）
    std::unordered_map<std::string, std::string> m_request_headers; // 请求头（如Cookie，Content-Type）
    std::unordered_map<std::string, std::string> m_form_data;   // 保存post请求表单内容

    ByteRange m_range;          // 请求的range

    // === 登陆信息 ===
    std::string m_username;       // 登陆的用户名
    bool m_authenticated;       // 是否登陆认证成功
    // TODO: 
    // m_request_json是否有效
};