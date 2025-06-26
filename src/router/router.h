#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include "session/session_manager.h"


class HttpConn;
class HttpRequest;
class HttpResponse;
// using HttpHandler = std::function<void(HttpConn&)>;
using HttpHandler = std::function<void(HttpRequest&, HttpResponse&)>;

class WebSocketConn;
using WebSocketHandler = std::function<void(WebSocketConn&, SessionId&, const std::string&)>;

class Router {
public:
    // === Http 相关 ===
    // 注册Get请求处理函数
    void registerGet(const std::string& path, HttpHandler handler);
    // 注册 POST 请求处理函数
    void registerPost(const std::string& path, HttpHandler handler);
    // 调用处理函数（根据路径与方法分发）
    void dispatch(const std::string& method, const std::string& path, HttpConn& conn);
    void dispatch(const std::string& method, const std::string& path, HttpRequest& http_request, HttpResponse& http_response);

    // === WebSocket 相关 ===
    void registerWebSocket(const std::string& path, WebSocketHandler handler);
    void dispatch(const std::string& path, WebSocketConn& conn, SessionId& sessionId, const std::string& msg);

private:
    // 路径 -> 回调函数 映射表
    std::unordered_map<std::string, HttpHandler>     m_get_routes;
    std::unordered_map<std::string, HttpHandler>     m_post_routes;
    std::unordered_map<std::string, WebSocketHandler> m_ws_routes;
};