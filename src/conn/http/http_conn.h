#pragma once

#include "conn/base_conn.h"
#include <netinet/in.h>
#include <string>
#include <unordered_map>
#include <functional>
#include "router/global_router.h"
#include "http_base.h"
#include "http_request.h"
#include "http_response.h"
#include "third_party/nlohmann/json.hpp"
 
using Json = nlohmann::json;

using EpollWriteCallback = std::function<void(int fd)>;

/*
HttpConn类：用于管理一个客户端连接（包括fd、地址、缓冲区等）
*/
class HttpConn  : public BaseConn
{
public:
    // 解析请求的状态机状态
    enum ParseState {
        PARSE_REQUEST_LINE,   // 正在解析请求行
        PARSE_HEADERS,        // 正在解析头部字段
        PARSE_BODY,           // 正在解析请求体（POST）
        PARSE_DONE            // 解析完成
    };

    // 请求类型
    enum HttpMethod {
        METHOD_GET,
        METHOD_POST,
        METHOD_HEAD,
        METHOD_PUT,
        METHOD_DELETE,
        METHOD_UNKNOWN
    };

    

public:
    HttpConn(/* args */);
    ~HttpConn();

    // 初始化连接
    void init(int sockfd = -1, const sockaddr_in& addr = sockaddr_in()) override;
    // 关闭连接
    void closeConn() override;
    // 是否已关闭连接
    bool isClosed() override;
    // 读数据
    bool read() override;
    // 发送响应
    WriteStatus write() override;  
    // 处理数据
    bool process() override;
    // 设置注册写事件的回调函数
    void setEpollWriteCallback(EpollWriteCallback cb) override;
    // === Getter ===
    int getSockFd() const override;
    sockaddr_in getAddr() const override;
    // const std::unordered_map<std::string, std::string>& getHeaders() const { return m_headers; }
    bool getKeepAlive() const override;

    // 获取全局路由
    static inline Router& getRouter() {
        return GlobalRouter::getInstance();
    }
private:
    // 获取基本输出信息
    std::string getBaseText();
    // TODO: request: 
    // 解析Http请求
    bool parseHttpRequest();
    // 从m_read_buf中移除前len个字节
    void removeLine(int len);
    // 关闭之前打开的文件
    void CloseFile();
    // 处理请求  
    bool handleRequest();
    // 处理写事件
    // bool processWrite();
    // 根据解析完成的信息，构造响应
    bool generateResponse();  
    bool copyResponseData();
    // 刷新session
    void refreshSession();
    
private:
    static const int READ_BUFFER_SIZE = 4096;    // 读缓冲区大小
    char m_read_buf[READ_BUFFER_SIZE];           // 读缓冲区
    int m_read_idx;                              // 读缓冲区指针（下一个写入位置）
    static const int WRITE_BUFFER_SIZE = 4096;   // 读缓冲区大小
    char m_write_buf[WRITE_BUFFER_SIZE];         // 响应写缓冲区
    int m_write_idx;                             // 写缓冲区指针

    ParseState m_parse_state;      // 当前解析状态

    std::shared_ptr<HttpBase> m_http_base;  // 保存基本http信息
    HttpRequest m_http_request;
    HttpResponse m_http_response;

    static const std::string kWebRoot;  // 网站根目录
    std::string m_file_path;    // m_url映射后的实际文件路径
    int m_file_fd = -1;             // 请求的文件描述符
    int m_file_response_size = 0;   // 实际需要传输的响应部分字节数（受 Range 影响）
    int m_file_total_size = 0;      // 请求的文件的总大小
    int m_file_bytes_sent = 0;           // 已发送正文的字节数
    int m_file_offset = 0;          // 文件起始发送偏移
    bool m_header_sent = false;     // 是否已经发完header
    bool m_response_built = false;  // 响应是否已经构建好            
    
    std::unordered_map<std::string, std::string> m_response_cookies;    // 保存响应cookie
    // static Router* m_router;        // 所有连接共享的路由器
};

