#pragma once

#include <functional>
#include <string>
#include <unordered_map>

// 路由处理函数类型：函数接受HttpConn的引用
class HttpConn;
class HttpRequest;
class HttpResponse;
// using RouteHandler = std::function<void(HttpConn&)>;
using RouteHandler = std::function<void(HttpRequest&, HttpResponse&)>;

class Router {
public:
    // 注册Get请求处理函数
    void registerGet(const std::string& path, RouteHandler handler);
    // 注册 POST 请求处理函数
    void registerPost(const std::string& path, RouteHandler handler);
    // 调用处理函数（根据路径与方法分发）
    void dispatch(const std::string& method, const std::string& path, HttpConn& conn);
    void dispatch(const std::string& method, const std::string& path, HttpRequest& http_request, HttpResponse& http_response);

private:
    // 路径 -> 回调函数 映射表
    std::unordered_map<std::string, RouteHandler>     m_get_routes;
    std::unordered_map<std::string, RouteHandler>     m_post_routes;
};