#pragma once

#include "router/global_router.h"
#include "conn/http/http_conn.h"

// 注册与用户相关的所有路由（登陆、注册）
class UserController {

public:
    static void registerRoutes() {
        Router& router = GlobalRouter::getInstance();

        router.registerPost("/login", login);
        router.registerPost("/register", registerUser);
        router.registerGet("/checktoken", checkToken);
    }    

private:
    // 登陆
    // static void login(HttpConn& conn);
    static void login(HttpRequest& http_request, HttpResponse& http_response);
    // 注册
    // static void registerUser(HttpConn& conn);
    static void registerUser(HttpRequest& http_request, HttpResponse& http_response);

    // 测试token
    static void checkToken(HttpRequest& http_request, HttpResponse& http_response);
};