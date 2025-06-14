#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include "http_base.h"
#include "session/session_manager.h"
#include "token/token_manager.h"


class HttpRequest {

public:
    HttpRequest();
    void init(std::shared_ptr<HttpBase> http_base);

    // === Getter ===
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
    const Token getToken() const;
    const ByteRange getRange() const;
    const std::string& getIp() const;
    
    // === Setter ===
    void setMethod(const std::string& method);
    void setUrl(const std::string& url);
    void setVersion(const std::string& version);
    void setBody(const std::string& body);
    void setKeepAlive(bool keep_alive);
    void setConnection(const std::string& connection);
    void setContentLength(int content_length);
    void setHeader(const std::string& key, const std::string& value);


    // === 快捷访问 ===
    const std::string getHeader(const std::string& key) const;  
    const std::string& getParam(const std::string& key) const;
    std::string getRequestCookie(const std::string& key) const;
    bool isAuthenticated() const;

private:
    friend class HttpConn;

    // === 仅供 HttpConn 调用的解析器接口 ===
    // 解析请求行
    bool parseRequestLine(const std::string& line);
    // 解析请求头
    bool parseHeaders(const std::string& line);
    // 解析请求体
    bool parseBody(char* read_buf, int content_length);
    // 提取POST请求中的表单字段（application/x-www-form-urlencoded）
    void parseFormData();
    // 提取POST请求中的表单字段（application/json）
    void parseJsonBody();
    // 解析身份信息（Session、Cookie、Token）
    void parseAuthContext();

private:
    std::shared_ptr<HttpBase> m_http_base;  // 保存基本的http属性
    
};