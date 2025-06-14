#include "user_controller.h"
#include "mvc/service/user_service.h"
#include "session/session_manager.h"
#include "token/token_manager.h"
#include "util/utils.h"

const std::string BASE_TEXT = "[UserController] ";
// void UserController::login(HttpConn& conn) {
//     auto form_data = conn.parseFormData(conn.getBody());
//     std::string username = form_data["username"];
//     std::string password = form_data["password"];
//     std::string new_url = UserService::login(username, password)?"/welcome.html":"/login_error.html";
//     SessionId session_id = SessionManager::getInstance().createSession();
//     conn.setCookie("session_id", session_id);
//     conn.setUrl(new_url);
// }

// void UserController::login(HttpConn& conn) {
void UserController::login(HttpRequest& http_request, HttpResponse& http_response) {
    std::string ip = http_request.getIp();
    if (RateLimiter::getInstance().isLimit("login_ip", ip)) {
        http_response.sendRESTfulJson(429, "请求过于频繁，请稍后再试！");
        return;
    }
    auto form_data = http_request.getFormData();
    std::string username = form_data["username"];
    std::string password = form_data["password"];
    std::string new_url;
    if (UserService::login(username, password)) {
        new_url = "/welcome.html";
        SessionId session_id = SessionManager::getInstance().createSession();
        // conn.setCookie("session_id", session_id);
        Token token = TokenManager::getInstance().createToken(username);
        Json data;
        data["code"] = 0;
        data["message"] = "登陆成功";
        data["token"] = token;
        LOG_INFO(BASE_TEXT + token);
        http_response.sendRESTfulJson(0, "OK", data);
    }
    else {
        new_url = "/login_error.html";
    }
    
    // http_response.setNewUrl(new_url);
}

// void UserController::registerUser(HttpConn& conn) {
void UserController::registerUser(HttpRequest& http_request, HttpResponse& http_response) {
    auto form_data = http_request.getFormData();
    std::string username = form_data["username"];
    std::string password = form_data["password"];
    std::string new_url = UserService::registerUser(username, password)? "/register_success.html": "/register_error.html";
    http_response.sendRedirect(new_url);
}

void UserController::checkToken(HttpRequest& http_request, HttpResponse& http_response) {
    if (!http_request.isAuthenticated()) {
        std::string s = "";
        http_response.sendRESTfulJson(401, "未登录", s);
        LOG_INFO(BASE_TEXT + "token = 无效");    
        return;
    }
    Token token = http_request.getToken();
    std::string username = http_request.getUsername();
    LOG_INFO(BASE_TEXT + "token = " + token + ", username = " + username);

    Json data;
    data["token"] = token;
    data["username"] = username;

    http_response.sendRESTfulJson(0, "Token有效", data);
}