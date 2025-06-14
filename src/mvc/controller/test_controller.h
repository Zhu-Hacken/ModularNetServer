#pragma once

#include "router/global_router.h"
#include "conn/http/http_conn.h"

class TestController {
public:
    static void registerRoutes() {
        Router& router = GlobalRouter::getInstance();

        // 注册一个 GET 请求的 JSON 接口
        router.registerGet("/api/hello", [](HttpRequest& http_request, HttpResponse& http_response){
            // std::string json = R"({"message":"Hello from API"})";
            Json data;
            data["code"] = "200";
            data["message"] = "Hello from API";
            http_response.sendJson(200, data);
        });

        router.registerGet("/api/time", [](HttpRequest& http_request, HttpResponse& http_response){
            time_t now= time(nullptr);
            char buffer[64];
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));

            Json data;
            data["status"] = "200";
            data["message"] = "successs";
            data["data"] = {
                {"timestamp", now},
                {"datetime", buffer}
            };
            http_response.sendJson(200, data);
        });

        

        router.registerPost("/api/echo", [](HttpRequest& http_request, HttpResponse& http_response) {
            // 简单回显请求体
            std::string body = http_request.getBody();
            http_response.sendJson(200, body);
        });
    }
};