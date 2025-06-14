#include "http_response.h"

void HttpResponse::init(std::shared_ptr<HttpBase> http_base) {
    m_http_base = http_base;
    m_status_code = 200;     // 默认 200 OK
    m_status_text = "OK";    
    m_response_body.clear(); // 清空响应体
    m_response_headers.clear(); // 清空响应头字段
    m_response_cookies.clear(); // 清空响应Cookie字段
    m_response_data.clear();    // 清空最终序列化的数据
}

bool HttpResponse::isBuilt() {
    return !m_response_data.empty();
}

// === Setter ===
void HttpResponse::setStatus(int code, const std::string& status_text) {
    m_status_code = code;
    m_status_text = status_text;
}

void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    m_response_headers[key] = value;
}

void HttpResponse::setCookie(const std::string& key, const std::string& value) {
    m_response_cookies[key] = value;
}

void HttpResponse::setBody(const std::string& body) {
    m_response_body = body;
}

void HttpResponse::setContentType(const std::string& content_type) {
    setHeader("Content-Type", content_type);
}

void HttpResponse::setKeepAlive(bool keep_alive) {
    if (keep_alive) {
        setHeader("Connection", "keep-alive");
        setHeader("Keep-Alive", "timeout=5");
    } else {
        setHeader("Connection", "close");
    }
}

void HttpResponse::sendRedirect(const std::string& new_url) {
    m_response_url = new_url;
    // setStatus(302, "Found");
    // setHeader("Location", new_url);
}

void HttpResponse::setNewUrl(const std::string& new_url) {
    m_response_url = new_url;
}

// === Getter ===
const std::string& HttpResponse::getUrl() const {
    return m_response_url;
}


Json HttpResponse::buildRESTfulJson(int status_code, const std::string& message, const std::string& data) {
    Json json;
    json["code"] = status_code;
    json["message"] = message;
    if (data.empty()) {
        json["data"] = nullptr;
    }
    else {
        json["data"] = data;
    }
    return json;
}

Json HttpResponse::buildRESTfulJson(int status_code, const std::string& message, const Json& data) {
    Json json;
    json["code"] = status_code;
    json["message"] = message;
    if (data.empty() || data.is_null()) {
        json["data"] = nullptr;
    }
    else {
        json["data"] = data;
    }
    return json;
}

void HttpResponse::sendText(int status_code, const std::string& title, const std::string& message) {
    std::string html = "<html><head><title>" + title + "</title></head>";
    html += "<body><h1>" + std::to_string(status_code) + " " + title + "</h1>";
    html += "<p>" + message + "</p></body></html>";

    int header_len = buildResponseHeader(status_code, title, "text/html", html.size());

    // 写入发送缓冲区
    // if (header_len + html.size() >= WRITE_BUFFER_SIZE) {
    //     LOG_ERROR(getBaseText() + "sendText 响应体过大，超过缓冲区大小");
    //     return;
    // }

    
    m_response_data += html;
    // memcpy(m_write_buf + header_len, html.c_str(), html.size());
    // m_write_idx += html.size();
    
    // 修改状态，确保触发写事件
    // m_response_built = true;
}

void HttpResponse::sendJson(int status_code, const std::string& json_str) {
    // m_write_idx = 0;
    m_response_data.clear();
    buildResponseHeader(status_code, "OK", "application/json", json_str.size());

    // if (json_str.size() + m_write_idx >= WRITE_BUFFER_SIZE) {
    //     LOG_ERROR(getBaseText() + "sendJson响应体过大，超过缓冲区大小。");
    //     return;
    // }

    m_response_data += json_str;

    // memcpy(m_write_buf + m_write_idx, json_str.c_str(), json_str.size())    ;
    // m_write_idx += json_str.size();
}

void HttpResponse::sendJson(int status_code, const Json& data) {
    std::string json_str = data.dump();
    sendJson(status_code, json_str);
}

void HttpResponse::sendRESTfulJson(int status_code, const std::string& message, const std::string& data) {
    Json json = buildRESTfulJson(status_code, message, data);
    sendJson(200, json);
}
void HttpResponse::sendRESTfulJson(int status_code, const std::string& message, const Json& data) {
    Json json = buildRESTfulJson(status_code, message, data);
    sendJson(200, json);
}

int HttpResponse::buildResponseHeader(int status_code, const std::string& status_text,
                                  const std::string& content_type, int content_length,
                                  const std::unordered_map<std::string, std::string>& extra_headers) 
{
    // m_write_idx = 0;
    m_response_data.clear();
    // 状态行
    // m_write_idx += snprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - m_write_idx, 
    //                         "HTTP/1.1 %d %s\r\n", status_code, status_text.c_str());
    m_response_data += "HTTP/1.1 " + std::to_string(status_code) + " " + status_text + "\r\n";
    // 头部字段
    m_response_data += "Content-Type: " + content_type + "\r\n";
    m_response_data += "Content-Length: " + std::to_string(content_length) + "\r\n";
    // m_write_idx += snprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - m_write_idx, 
    //                         "Content-Type: %s\r\n", content_type.c_str());

    // m_write_idx += snprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - m_write_idx, 
    //                         "Content-Length: %d\r\n", content_length);

    for (const auto& kv: m_response_cookies) {
        // m_write_idx += snprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - m_write_idx, 
        //                     "Set-Cookie: %s=%s; Path=/; Max-Age=1800\r\n", kv.first.c_str(), kv.second.c_str());          
        m_response_data += "Set-Cookie: " + kv.first + "=" + kv.second + "; Path=/; Max-Age=1800\r\n";
    }

    if (m_http_base->getKeepAlive()) {
        // m_write_idx += snprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - m_write_idx, 
        //                     "Connection: keep-alive\r\n");       
        // m_write_idx += snprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - m_write_idx, 
        //                     "Keep-Alive: timeout=5\r\n");       
        m_response_data += "Connection: keep-alive\r\n";
        m_response_data += "Keep-Alive: timeout=5\r\n";
    } else {
        // m_write_idx += snprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - m_write_idx, 
        //                     "Connection: close\r\n");       
        m_response_data += "Connection: close\r\n";                            
    }
    for (const auto& it : m_response_headers) {
        m_response_data += it.first + ": " + it.second + "\r\n";
    }
    // 额外header
    for (const auto& it : extra_headers) {
        m_response_data += it.first + ": " + it.second + "\r\n";
    }
    // for (std::unordered_map<std::string, std::string>::const_iterator it = extra_headers.begin(); it != extra_headers.end(); ++it ) {
    //     m_write_idx += snprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - m_write_idx, 
    //                             "%s: %s\r\n", it->first.c_str(), it->second.c_str());       
    // }

    // 空行                                                 
    m_response_data += "\r\n";
    // m_write_idx += snprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - m_write_idx, 
    //                         "\r\n");       
    return m_response_data.size();
}

const std::string& HttpResponse::getResponseData() const {
    return m_response_data;
}