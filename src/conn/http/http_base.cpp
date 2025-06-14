#include "http_base.h"

void HttpBase::init(std::string ip) {
    m_method.clear();       
    m_url.clear();          
    m_version.clear();      
    m_request_body.clear(); 
    m_keep_alive = false;   
    m_connection.clear();   
    m_content_length = 0;      
    m_request_headers.clear(); 
    m_form_data.clear();
    m_username.clear();
    m_authenticated = false;
    m_range = {-1,-1,false};
    m_ip = ip;
}

// === Getter ===
const std::string& HttpBase::getIp() const {return m_ip; }
const std::string& HttpBase::getMethod() const { return m_method; }
const std::string& HttpBase::getUrl() const { return m_url; }
const std::string& HttpBase::getVersion() const { return m_version; }
const std::string& HttpBase::getBody() const { return m_request_body; }
const Json& HttpBase::getJson() const {return m_request_json;};
bool HttpBase::getKeepAlive() const { return m_keep_alive; }
const std::string& HttpBase::getConnection() const { return m_connection; }
int HttpBase::getContentLength() const { return m_content_length; }
const std::unordered_map<std::string, std::string>& HttpBase::getHeaders() const { return m_request_headers; }
const std::unordered_map<std::string, std::string>& HttpBase::getFormData() const { return m_form_data; }
const std::string& HttpBase::getUsername() const {return m_username;}
bool HttpBase::isAuthenticated() const {return m_authenticated;}
const ByteRange HttpBase::getRange() const {return m_range;}

void HttpBase::parseRange() {
    /*
        Range: bytes=100-200
        Range: bytes=500-
        Range: bytes=-300    
    */
    std::string value = getHeader("Range");
    std::string prefix = "bytes=";
    // ByteRange range{-1, -1, false};
    if (value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0) {
        std::string range_str = value.substr(value.find('=') + 1);

        size_t pos = range_str.find('-');
        if (pos == std::string::npos) {
            m_range.isValid = false;
            return;
        }

        std::string start_str = range_str.substr(0, pos);
        std::string end_str = range_str.substr(pos + 1);

        try {
            if (!start_str.empty()) m_range.start = std::stoi(start_str);
            if (!end_str.empty()) m_range.end = std::stoi(end_str);
            m_range.isValid = true;
        } catch (...) {
            m_range.isValid = false;
        }
    }
}

// === Setter ===
void HttpBase::setIp(const std::string& ip) {m_ip = ip;}
void HttpBase::setMethod(const std::string& method) { m_method = method; }
void HttpBase::setUrl(const std::string& url) { m_url = url; }
void HttpBase::setVersion(const std::string& version) { m_version = version; }
void HttpBase::setBody(const std::string& body) { m_request_body = body; }
void HttpBase::setJson(const Json&& json) {m_request_json = json;}
void HttpBase::setKeepAlive(bool keep_alive) { m_keep_alive = keep_alive; }
void HttpBase::setConnection(const std::string& connection) { m_connection = connection; }
void HttpBase::setContentLength(int content_length) { m_content_length = content_length; }
void HttpBase::setHeader(const std::string& key, const std::string& value) { m_request_headers[key] = value; }
void HttpBase::setFormData(std::unordered_map<std::string, std::string>&& form_data) {m_form_data = std::move(form_data);}
void HttpBase::setUsername(const std::string& username) {
    m_username = username;
    m_authenticated = true;
}

std::string HttpBase::getHeader(const std::string& key) const {
    auto it = m_request_headers.find(key);
    if (it == m_request_headers.end()) {
        return "";
    }
    return it->second;
}