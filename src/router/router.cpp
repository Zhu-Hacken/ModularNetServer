#include "conn/http/http_conn.h"
#include "conn/http/http_request.h"
#include "conn/http/http_response.h"
#include "util/utils.h"
#include "log/logs.h"

const std::string BASE_TEXT = "[Router] ";

// 注册Get请求处理函数
void Router::registerGet(const std::string& path, HttpHandler handler) {
    LOG_INFO(BASE_TEXT + "get注册路径: " + path);
    m_get_routes[path] = handler;
}

// 注册 POST 请求处理函数
void Router::registerPost(const std::string& path, HttpHandler handler) {
    LOG_INFO(BASE_TEXT + "post注册路径: " + path);
    m_post_routes[path] = handler;
}

// 调用处理函数（根据路径与方法分发）
// void Router::dispatch(const std::string& method, const std::string& path, HttpConn& conn) {
//     LOG_INFO(BASE_TEXT + "当前method = " + method + "，path = " + path);
//     if (method == "GET") {
//         auto it = m_get_routes.find(path);
//         if (it != m_get_routes.end()) {
//             it->second(conn);       // 调用处理函数
//             return;
//         }
//     }
//     else if (method == "POST") {
//         auto it = m_post_routes.find(path);
//         if (it != m_post_routes.end()) {
//             it->second(conn);
//             return;
//         }
//     }

//     if (method == "GET") {
//         LOG_INFO(BASE_TEXT + "GET " + path + " 未注册，默认走静态页面");
//         return;
//     }

//     // 未找到路由，发送404页面
//     conn.sendText(404, "Not Found", "The requested URL was not found on this server.");
// }

// 调用处理函数（根据路径与方法分发）
void Router::dispatch(const std::string& method, const std::string& path, HttpRequest& http_request, HttpResponse& http_response) {
    LOG_INFO(BASE_TEXT + "当前method = " + method + "，path = " + path);
    if (method == "GET") {
        auto it = m_get_routes.find(path);
        if (it != m_get_routes.end()) {
            it->second(http_request, http_response);       // 调用处理函数
            return;
        }
    }
    else if (method == "POST") {
        auto it = m_post_routes.find(path);
        if (it != m_post_routes.end()) {
            it->second(http_request, http_response);
            return;
        }
    }

    if (method == "GET") {
        LOG_INFO(BASE_TEXT + "GET " + path + " 未注册，默认走静态页面");
        http_response.setNewUrl(http_request.getUrl());
        return;
    }

    // 未找到路由，发送404页面
    http_response.sendText(404, "Not Found", "The requested URL was not found on this server.");
}

void Router::registerWebSocket(const std::string& path, WebSocketHandler handler) {
    LOG_INFO(BASE_TEXT + "WebSocket 注册路径：" + path);
    m_ws_routes[path] = handler;
}

void Router::dispatch(const std::string& path, WebSocketConn& conn, SessionId& sessionId, const std::string& msg) {
    LOG_INFO(BASE_TEXT + "当前 path = " + path);
    auto it = m_ws_routes.find(path);
    if (it != m_ws_routes.end()) {
        it->second(conn, sessionId, msg);
        return;
    }
}