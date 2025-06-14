#include "conn/http/http_request.h"
#include "util/utils.h"
#include <sstream>


const std::string BASE_TEXT = "[HttpRequest] ";

HttpRequest::HttpRequest() {

}

void HttpRequest::init(std::shared_ptr<HttpBase> http_base) {
    m_http_base = http_base;
}

// === Getter ===
const std::string& HttpRequest::getMethod() const { return m_http_base->getMethod(); }
const std::string& HttpRequest::getUrl() const { return m_http_base->getUrl(); }
const std::string& HttpRequest::getVersion() const { return m_http_base->getVersion(); }
const std::string& HttpRequest::getBody() const { return m_http_base->getBody(); }
const Json& HttpRequest::getJson() const {return m_http_base->getJson();};
bool HttpRequest::getKeepAlive() const { return m_http_base->getKeepAlive(); }
const std::string& HttpRequest::getConnection() const { return m_http_base->getConnection(); }
int HttpRequest::getContentLength() const { return m_http_base->getContentLength(); }
const std::unordered_map<std::string, std::string>& HttpRequest::getHeaders() const { return m_http_base->getHeaders(); }
const std::unordered_map<std::string, std::string>& HttpRequest::getFormData() const { return m_http_base->getFormData(); }
const std::string& HttpRequest::getUsername() const {return m_http_base->getUsername();}
const Token HttpRequest::getToken() const {return getHeader("Authorization");}
const ByteRange HttpRequest::getRange() const {return m_http_base->getRange();}
const std::string& HttpRequest::getIp() const {return m_http_base->getIp(); }
// === Setter ===
void HttpRequest::setMethod(const std::string& method) { m_http_base->setMethod(method); }
void HttpRequest::setUrl(const std::string& url) { m_http_base->setUrl(url); }
void HttpRequest::setVersion(const std::string& version) { m_http_base->setVersion(version); }
void HttpRequest::setBody(const std::string& body) { m_http_base->setBody(body); }
void HttpRequest::setKeepAlive(bool keep_alive) { m_http_base->setKeepAlive(keep_alive); }
void HttpRequest::setConnection(const std::string& connection) { m_http_base->setConnection(connection); }
void HttpRequest::setContentLength(int content_length) { m_http_base->setContentLength(content_length); }
void HttpRequest::setHeader(const std::string& key, const std::string& value) { m_http_base->setHeader(key, value); }

const std::string HttpRequest::getHeader(const std::string& key) const {
    return m_http_base->getHeader(key);
}

std::string HttpRequest::getRequestCookie(const std::string& key) const {
    /*
    Cookie: session_id=sess_123456; Path=/; Max-Age=1800
    */
    const auto& headers = m_http_base->getHeaders();


    auto it = headers.find("Cookie");
    if (it == headers.end()) {
        return "";
    }


    std::string cookie_header = it->second;
    std::istringstream stream(cookie_header);
    std::string pair;
    while (std::getline(stream, pair, ';')) {
        size_t pos = pair.find('=');
        if (pos != std::string::npos) {
            std::string cookie_key = pair.substr(0, pos);
            std::string cookie_value = pair.substr(pos+1);
            // 去除可能存在的空格
            while (!cookie_key.empty() && cookie_key.front() == ' ') {
                cookie_key.erase(0, 1);
            }
            if (cookie_key == key) {
                return cookie_value;
            }
        }
    }
    return "";
}

bool HttpRequest::parseRequestLine(const std::string& line) {
    std::string method, url, version;
    std::istringstream iss(line);   //  转成流对象
    iss >> method >> url >> version;  // 用空格分割内容

    if(method.empty() || url.empty() || version.empty()) {
        LOG_ERROR(BASE_TEXT + "请求行解析失败：" + line);
        return false;
    }

    m_http_base->setMethod(method);
    m_http_base->setUrl(url);
    m_http_base->setVersion(version);

    LOG_DEBUG(BASE_TEXT + "解析请求行成功：方法 = " + method
              + "，路径 = " + url
              + "，协议 = " + version);
    return true;
}

bool HttpRequest::parseHeaders(const std::string& line) {
    if (line == "\r\n" || line == "") {
        // 空行表示headers结束
        return false;   // 通知状态机：header解析结束
    }
    
    size_t pos = line.find(':');
    if(pos == std::string::npos) {
        LOG_ERROR(BASE_TEXT + "Header格式非法：" + line);
        return true;    // 忽略错误行，继续
    }
    
    std::string key = line.substr(0, pos);
    std::string value = line.substr(pos + 1);

    // 去除可能存在的空格
    while(!value.empty() && (value[0] == ' ' || value[0] == '\t')) {
        value.erase(0, 1);
    }

    // 移除行尾的\r\n
    if(!value.empty() && value.back() == '\r') {
        value.pop_back();
    }
    std::unordered_map<std::string, std::string> request_headers; // 请求头（如Cookie，Content-Type）
    int content_length;
    std::string connection;
    bool keep_alive;

    request_headers[key] = value;
    m_http_base->setHeader(key, value);

    if(key == "Content-Length") {
        content_length = std::stoi(value);
        m_http_base->setContentLength(content_length);
    }

    if(key == "connection") {
        connection = value;
        // 判断是否保持连接
        keep_alive = (connection == "keep-alive");
        m_http_base->setConnection(connection);
        m_http_base->setKeepAlive(keep_alive);
    }
    if(key == "Range") {
        m_http_base->parseRange();
    }

    LOG_DEBUG(BASE_TEXT + "Header解析成功：" + key + " = " + value);

    return true;
}

bool HttpRequest::parseBody(char* read_buf, int content_length) {
    std::string body = std::string(read_buf, content_length);
    m_http_base->setBody(body);

    std::string contentType = m_http_base->getHeader("Content-Type");
    if (contentType.find("application/x-www-form-urlencoded") != std::string::npos) {
        parseFormData();
        LOG_INFO(BASE_TEXT + "parseFormData()");
    } else if (contentType.find("application/json") != std::string::npos) {
        parseJsonBody();
        LOG_INFO(BASE_TEXT + "parseJsonBody()");
    } else {

    }
    return true;
}

void HttpRequest::parseFormData() {
    std::unordered_map<std::string, std::string> result;
    std::istringstream ss(m_http_base->getBody());
    std::string kv;
    while (std::getline(ss, kv, '&')) {
        size_t pos = kv.find('=');
        if (pos != std::string::npos) {
            std::string key = kv.substr(0, pos);
            std::string value = kv.substr(pos + 1);
            result[key] = value;
        }
    }
    m_http_base->setFormData(std::move(result));
}

void HttpRequest::parseJsonBody() {
    try {
        const std::string& body = m_http_base->getBody();
        Json json = nlohmann::json::parse(body);
        m_http_base->setJson(std::move(json));

    } catch (const std::exception& e) {
        LOG_WARN("Invalid JSON body: " + std::string(e.what()));
    }
}

void HttpRequest::parseAuthContext() {
    LOG_INFO("[HttpRequest] 解析用户身份...");
    SessionId sessionId = getRequestCookie("session_id");
    if (SessionManager::getInstance().isSessionIdValid(sessionId)) {
        if (!SessionManager::getInstance().isSessionIdExpired(sessionId)) {
            // std::string username = SessionManager::getInstance().get(const SessionId &id, const std::string &key)
            // sessionId有效
            // m_http_base->setUsername()
        }
    }

    Token token = getToken();
    if (TokenManager::getInstance().isTokenValid(token)) {
        if (!TokenManager::getInstance().isTokenExpired(token)) {
            std::string username = TokenManager::getInstance().getUsername(token);
            m_http_base->setUsername(username);
            LOG_INFO("[HttpRequest] 已解析用户身份: " + getUsername());
        }
    }
    LOG_INFO("[HttpRequest] 解析用户身份完毕。");
}

bool HttpRequest::isAuthenticated() const {
    return m_http_base->isAuthenticated();
}