#pragma once

#include "conn/http/http_conn.h"
#include "router/global_router.h"

class TestTxController{
public:
    static void registerRoutes() {
        GlobalRouter::getInstance().registerPost("/api/tx_test", handle);
    }

public:
    static void handle(HttpRequest& http_request, HttpResponse& http_response);
};