#include "http_conn.h"
#include "util/utils.h"
#include "config/configs.h"
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <sys/stat.h>
#include <fcntl.h>
#include "session/session_manager.h"
#include <arpa/inet.h>
#include <sys/sendfile.h>
#include "log/logs.h"

// === 工作函数区 ===

bool endsWith(const std::string& str, const std::string& suffix) {
    // 判断字符串str是否以suffix结尾
    if(str.size() < suffix.size()) return false;
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}


std::string getMimeType(const std::string& path) {
    // 获取 MIME 类型的函数
    if(endsWith(path, ".html")) return "text/html; charset=utf-8";
    if(endsWith(path, ".css")) return "text/css";
    if(endsWith(path, ".js")) return "application/javascript";
    if(endsWith(path, ".jpg") || endsWith(path, ".jpeg")) return "image/jpeg";
    if(endsWith(path, ".png")) return "image/png";
    if(endsWith(path, ".gif")) return "image/gif";
    if(endsWith(path, ".mp4")) return "video/mp4";
    return "application/octet-stream";
}


// === HttpConn成员区 ===
// int HttpConn::m_trig_mode = ServerConfig::ET;

// Router* HttpConn::m_router = nullptr;

std::string HttpConn::getBaseText() {
    return "[HttpConn] [Thread-"+ SysUtils::getThreadIdStr() +"] m_sockfd = " + std::to_string(m_sockfd) + " ";
}

const std::string HttpConn::kWebRoot = SysUtils::getRootPath() + "/src/www"; // 网站根目录

HttpConn::HttpConn() : m_read_idx(0) {}

HttpConn::~HttpConn()
{
    closeConn();
}

void HttpConn::init(int sockfd, const sockaddr_in& addr) {
    if (sockfd != -1) {
        m_sockfd = sockfd;
        m_address = addr;
        LOG_INFO(getBaseText() + "init() 初始化连接状态。");
    } else{
        LOG_INFO(getBaseText() + "init() 重置连接状态。");
    }

    // m_connection.clear();
    // === 初始化 HttpBase + HttpRequest + HttpResponse
    m_http_base = std::make_shared<HttpBase>();
    m_http_base->init(inet_ntoa(m_address.sin_addr));
    m_http_request.init(m_http_base) ;
    m_http_response.init(m_http_base);

    // === 缓冲区清空 ===
    memset(m_read_buf, 0, READ_BUFFER_SIZE);
    m_read_idx = 0;
    memset(m_write_buf, 0, WRITE_BUFFER_SIZE);
    m_write_idx = 0;


    // === 状态机重置 ===
    m_parse_state = PARSE_REQUEST_LINE;

    // === 路径、参数、头部字段 ===
    // m_method = METHOD_UNKNOWN;
    // m_url.clear();
    // m_version.clear();
    // m_connection.clear();
    // m_content_length = 0;

    // === 响应状态 ===
    // m_keep_alive = false;
    m_header_sent = false;
    m_file_response_size = 0;
    m_file_total_size = 0;      
    m_file_bytes_sent = 0;
    m_file_offset = 0;
    m_response_built = false;
    CloseFile();

    // === 关闭状态 === 
    m_is_closed = false;


    // std::cout << "HttpConn init: fd = " << m_sockfd << "\n";
}

void HttpConn::closeConn() {
    if(m_is_closed.exchange(true)){
        return; // 已关闭，直接跳过
    }

    if (m_sockfd != -1) {
        LOG_INFOF("%sHttpConn closed: fd = %d", getBaseText().c_str(), m_sockfd);
        close(m_sockfd);
        m_sockfd = -1;
    }
}

bool HttpConn::isClosed()
{
    return m_is_closed;
}

bool HttpConn::read() {
    LOG_INFOF( "%s进入 read()，当前触发模式 = %s", getBaseText().c_str(), (m_trig_mode == ServerConfig::ET ? "ET" : "LT"));
    if (m_sockfd == -1) {
        LOG_ERROR(getBaseText() + "尝试对无效连接进行 read()，fd = -1");
        return false;
    }
    if (m_read_idx >= READ_BUFFER_SIZE) {
        LOG_ERROR(getBaseText() + "Read buffer overflow");
        return false;
    }

    int bytes_read = 0;

    if (m_trig_mode == ServerConfig::LT) {

        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);

        if (bytes_read == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 非阻塞读完了
                return false;
            }
            LOG_ERROR(getBaseText() + "recv() failed, errno = " + std::to_string(errno) );
            return false;
        } 
        else if ( bytes_read == 0){
            // 对方关闭连接
            return false;
        }

        m_read_idx += bytes_read;

        // LOG_DEBUG(getBaseText() + "read from fd = " + std::to_string(m_sockfd ) 
        //     + ": \n" + std::string(m_read_buf, m_read_idx));
        LOG_DEBUGF("%sread from fd = %d:\n%.*s", 
           getBaseText().c_str(), 
           m_sockfd, 
           m_read_idx, 
           m_read_buf);

        return true;

    }
    else if (m_trig_mode == ServerConfig::ET) {

        // ET模式，必须一次性读取完缓冲区
        while(true) {
            bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);

            if (bytes_read == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // 非阻塞读完了
                    break;
                }
                LOG_ERROR(getBaseText() + "recv() failed, errno = " + std::to_string(errno) );
                return false;
            } 
            else if ( bytes_read == 0){
                // 对方关闭连接
                return false;
            }

            m_read_idx += bytes_read;

            if (m_read_idx >= READ_BUFFER_SIZE) {
                // 如果缓冲区已满，退出循环
                break;
            }
        }
        // LOG_DEBUG(getBaseText() + "read from fd = " + std::to_string(m_sockfd ) 
        //     + ": \n" + std::string(m_read_buf, m_read_idx));
        LOG_DEBUGF("%sread from fd = %d:\n%.*s", 
           getBaseText().c_str(), 
           m_sockfd, 
           m_read_idx, 
           m_read_buf);

        return true;

    }
    return false;
}


WriteStatus HttpConn::write() {
    int write_buf_total_sent = 0; // 写缓冲区中已经发送的字节数
    if ( !m_header_sent) {
        // 1. 先发送写缓冲区的内容，通常是header+Json(如果有的话)
        while( write_buf_total_sent < m_write_idx) {
            int bytes_sent = send(m_sockfd, m_write_buf + write_buf_total_sent, m_write_idx - write_buf_total_sent, 0);
            if(bytes_sent <= 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // 非阻塞写缓冲区满，后续还需写入。TODO: 并未重新监听
                    LOG_ERROR(getBaseText() + "写缓冲区暂满，等待下一轮EPOLLOUT");
                    return WriteStatus::AGAIN;
                }
                LOG_ERROR(getBaseText() + "send失败，errno = " + std::to_string(errno));
                return WriteStatus::ERROR;
            }
            write_buf_total_sent += bytes_sent;
        }
        // std::cout << m_sockfd << ": in " << "write 2" << std::endl;
        m_header_sent = true;
    }

    // 2. 发送响应体（如果有文件需要发送的话）
    if (m_file_fd == -1) {
        LOG_INFOF( "%s动态响应发送完成（无文件）", getBaseText().c_str());
        return WriteStatus::OK;
    }

    if ( m_file_bytes_sent < m_file_response_size) {
        size_t count = m_file_response_size - m_file_bytes_sent;
        off_t offset = m_file_offset + m_file_bytes_sent;

        ssize_t sent = sendfile(m_sockfd, m_file_fd, &offset, count);
        if (sent > 0) {
            m_file_bytes_sent += sent;
            // LOG_INFO(getBaseText() + "Range 请求解析结果: start = " + std::to_string(m_http_request.getRange().start) +
            // ", end = " + std::to_string(m_http_request.getRange().end) +
            // ", file_offset = " + std::to_string(m_file_offset) +
            // ", response_size = " + std::to_string(m_file_response_size) +
            // ", file_total_size = " + std::to_string(m_file_total_size));
            LOG_INFOF("%sRange 请求解析结果: start = %d, end = %d, file_offset = %ld, response_size = %ld, file_total_size = %ld",
                getBaseText().c_str(),
                m_http_request.getRange().start,
                m_http_request.getRange().end,
                m_file_offset,
                m_file_response_size,
                m_file_total_size);


        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK){
            // TODO: 错误处理
            LOG_WARNF( "%ssendfile 缓冲区满", getBaseText().c_str());
            return WriteStatus::AGAIN;
        }
        else {
            LOG_WARN(getBaseText() + "sendfile 失败，errno = " + std::to_string(errno));
            return WriteStatus::ERROR;
        }
    }
 
    CloseFile();
    // LOG_INFO(getBaseText() + "响应发送成功，共" + std::to_string(write_buf_total_sent + m_file_bytes_sent) + "字节");
    LOG_INFOF("%s响应发送成功，共%ld字节",
          getBaseText().c_str(),
          static_cast<long>(write_buf_total_sent + m_file_bytes_sent));

    return WriteStatus::OK;
}


bool HttpConn::process() {
    LOG_INFOF( "%sProcessing fd = %d", getBaseText().c_str(), m_sockfd);

    /*
    经典HTTP报文：
        GET / HTTP/1.1\r\n
        Host: localhost\r\n
        Connection: keep-alive\r\n
        Content-Length: 10\r\n
        \r\n
    */
    std::string ip = inet_ntoa(m_address.sin_addr);
    if (!RateLimiter::getInstance().isLimit("access_ip", ip)) {
        // 状态机解析请求
        if (!parseHttpRequest()) return false;
        if (!handleRequest()) return false;

        refreshSession();
    } 
    else {
        // ip请求次数过多，已被限制
        LOG_WARN(getBaseText() + "请求频率过高，已被限制，ip = " + ip);
        m_http_response.sendRESTfulJson(429, "Too Many Requests");
        copyResponseData();
    }

    if(m_write_cb) m_write_cb(m_sockfd);   // 注册监听写事件
    return true;
}

void HttpConn::setEpollWriteCallback(EpollWriteCallback cb)
{
    m_write_cb = std::move(cb);
}


void HttpConn::removeLine(int len) {
    if(len >= m_read_idx) {
        m_read_idx = 0;
        m_read_buf[0] = '\0';
    }
    else {
        memmove(m_read_buf, m_read_buf + len, m_read_idx - len);
        m_read_idx -= len;
        m_read_buf[m_read_idx] = '\0';
    }
}

bool HttpConn::generateResponse()
{
    /*
    响应报文：
        HTTP/1.1 200 OK\r\n
        Content-Type: text/html\r\n
        Content-Length: 27\r\n
        \r\n
        <html>hello world</html>
    */
    
    m_write_idx = 0;    //  清空写缓冲区
    
    // 默认首页映射
    if (m_http_request.getUrl() == "/") {
        m_http_response.sendRedirect("/index.html");
    }

    std::string real_url = m_http_response.getUrl();

    // 构造完整路径
    m_file_path = kWebRoot + real_url;

    // 判断文件是否存在
    struct stat file_stat;      // 获取一个文件的元属性
    if(stat(m_file_path.c_str(), &file_stat) < 0) {
        LOG_ERROR(getBaseText() + "请求文件不存在：" + m_file_path);
        m_http_response.sendRedirect("/404.html");
        real_url = m_http_response.getUrl();
        m_file_path = kWebRoot + real_url;
        stat(m_file_path.c_str(), &file_stat); // 打开404页面
    }

    // 打开文件并读取内容作为响应体
    int file_size = file_stat.st_size;
    m_file_total_size = file_size;
    m_file_fd = open(m_file_path.c_str(), O_RDONLY);
    if (m_file_fd == -1) {
        LOG_ERROR(getBaseText() + "打开文件失败：" + m_file_path);
        return false;
    } else {
        // LOG_INFO(getBaseText() + "打开文件" + m_file_path + "，描述符 m_file_fd = " + std::to_string(m_file_fd));
        LOG_INFOF("%s打开文件%s，描述符 m_file_fd = %d",
          getBaseText().c_str(),
          m_file_path.c_str(),
          m_file_fd);

    }

    m_file_response_size = file_size;
    std::string mime = getMimeType(real_url);  

    int status_code = 0;
    std::string status_text = "";
    std::string content_type = mime;
    int content_length = m_file_response_size;

    // 构造响应头
    ByteRange range = m_http_request.getRange();
    if (range.isValid) {
        if ( range.start >= 0 && range.start < file_size) {
            m_file_offset = range.start;
            m_file_response_size = (range.end != -1 && range.end >= range.start) ? (range.end - range.start + 1): (file_size - range.start);
        }
        else if(range.start == -1 && range.end >0 && range.end <= file_size) {
            m_file_offset = file_size - range.end;
            m_file_response_size = range.end;
        } 
        else {
            // 非法 Range，返回416
            status_code = 416;
            status_text = "Range Not Satisfiable";
            content_length = 0;   
            m_file_offset = 0;
            m_file_response_size = 0; 
        }
        if (status_code != 416){
            status_code = 206;
            status_text = "Partial Content";
            content_length = m_file_response_size;
        }
    }
    else {
        status_code = 200;
        status_text = "OK";
        m_http_response.setHeader("Accept-Ranges", "bytes");
    }

    if (status_code == 206) {
        std::string content_range = "bytes " + std::to_string(m_file_offset) + "-" +
                                    std::to_string(m_file_offset + m_file_response_size - 1) + "/" +
                                    std::to_string(m_file_total_size);
        m_http_response.setHeader("Accept-Ranges", "bytes");
        m_http_response.setHeader("Content-Range", content_range);
    }



    m_http_response.buildResponseHeader(status_code, status_text, content_type, content_length);

    return copyResponseData();
}

bool HttpConn::copyResponseData() {
    const std::string& data = m_http_response.getResponseData();
    if (data.size() >= WRITE_BUFFER_SIZE) {
        LOG_ERROR(getBaseText() + "响应体过大，超过缓冲区");
        return false;
    }
    memcpy(m_write_buf, data.data(), data.size());
    m_write_idx = data.size();
    return true;
}


void HttpConn::refreshSession() {
    std::string session_id = m_http_request.getRequestCookie("session_id");
    if (session_id == "") return;
    if (SessionManager::getInstance().exists(session_id)){
        SessionManager::getInstance().refresh(session_id);
    }
}

void HttpConn::CloseFile() {
    if (m_file_fd != -1) {
        close(m_file_fd);
        m_file_fd = -1;
    }
}

// 处理读事件
bool HttpConn::parseHttpRequest() {
    while(true) {
        if(m_parse_state == PARSE_REQUEST_LINE) { 
            LOG_DEBUGF("%s-> 状态：解析请求行", getBaseText().c_str());

            char *crlf = strstr(m_read_buf, "\r\n");    // 查找请求行结束符
            if(!crlf) {
                LOG_ERROR(getBaseText() + "请求行未读完整，等待下一次epoll通知");
                return false;
            }

            // 构造std::string请求行
            std::string request_line(m_read_buf, crlf);

            // 调用解析请求行
            if(!m_http_request.parseRequestLine(request_line)) {
                LOG_ERROR(getBaseText() + "请求行解析失败，终止连接");
                return false;
            }

            int line_len = crlf - m_read_buf + 2;
            removeLine(line_len);

            m_parse_state = PARSE_HEADERS;
        }
        else if(m_parse_state == PARSE_HEADERS) {
            LOG_DEBUGF("%s-> 状态：解析请求头", getBaseText().c_str());
            
            while(true) {
                char* line_end = strstr(m_read_buf, "\r\n");
                if(!line_end) {
                    LOG_ERROR(getBaseText() + "请求头未读完整，等待下一次epoll通知");
                    return false;
                }

                std::string header_line(m_read_buf, line_end);

                // 将本行从缓冲区中移除（“消费数据”）
                int line_len = line_end - m_read_buf + 2;   // +2是\r\n长度
                removeLine(line_len);
                // 解析请求头
                if(!m_http_request.parseHeaders(header_line)) {
                    m_parse_state = PARSE_BODY;
                    break;
                }
            }
            
            // 判断是否保持连接
            // m_keep_alive = (m_connection == "keep-alive");

            m_parse_state = PARSE_BODY;
        }
        else if(m_parse_state == PARSE_BODY) {
            LOG_DEBUGF("%s-> 状态：解析请求体", getBaseText().c_str());
            // TODO: parseBody();
            int content_length = m_http_request.getContentLength();
            if(m_read_idx < content_length) {
                LOG_ERROR(getBaseText() + "请求体未读完整，等待更多数据");
                return false;
            }

            m_http_request.parseBody(m_read_buf, content_length);
            // m_body = std::string(m_read_buf, m_content_length);
            removeLine(content_length);   // 消费掉body

            LOG_DEBUGF("%s收到请求体：%s", getBaseText().c_str(), m_http_request.getBody().c_str());

            m_parse_state = PARSE_DONE;
        }
        else if(m_parse_state == PARSE_DONE) {
            LOG_DEBUGF("%s-> 状态：解析完成。", getBaseText().c_str());
            return true;
        }
    }
}

// 处理请求
bool HttpConn::handleRequest() {
    // getRouter().dispatch(m_http_request.getMethod(), m_http_request.getUrl(), *this);
    if (Interceptor::getInstance().shouldIntercept("uri_blacklist", m_http_request.getUrl())) {
        m_http_response.sendRESTfulJson(403, "禁止访问该资源！");
    } else {

        m_http_request.parseAuthContext();

        // if (!m_http_base->isAuthenticated() && RequestAuthInterceptor::isProtectedUrl(m_http_request.getUrl())) {
        if (!m_http_base->isAuthenticated() && Interceptor::getInstance().shouldIntercept("login_unrequired", m_http_request.getUrl())) {
            m_http_response.sendRESTfulJson(401, "未认证，请登录！");
        }
        else {
            getRouter().dispatch(m_http_request.getMethod(), m_http_request.getUrl(), m_http_request, m_http_response);
        }

    }
    // 情况1：已经写入响应（动态处理时构造了response）
    // if (m_write_idx > 0) return true;
    if (m_http_response.isBuilt()) {
        return copyResponseData();
    }

    // 情况2：未写入，尝试构造静态文件响应
    if(generateResponse()) return true; 

    // 情况3：两种都失败，关闭连接
    LOG_ERROR(getBaseText() + "响应构建失败");
    closeConn();
    return false;
}

// 处理写事件
// bool HttpConn::processWrite() {
//     // TODO: 未使用
//     return false;
// }

int HttpConn::getSockFd() const  {
    return m_sockfd;
}
sockaddr_in HttpConn::getAddr() const {
    return m_address;
}

bool HttpConn::getKeepAlive() const {
    return m_http_base->getKeepAlive();
}