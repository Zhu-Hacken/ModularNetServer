#include "interceptor_config.h"
#include "util/interceptor.h"

void InterceptorConfig::registerAllInterceptor() {
    Interceptor::getInstance().registerTypeRule("login_unrequired", {
    "/login.html",
    "/register",
    "/index.html",
    "/",
    "/logo.jpg",
    "/api/hello",
    "/login",
    "/welcome.html",
    "/video.mp4",
    "/test_transaction.html",
    "/api/tx_test"
    }, true);

     // 注册 URI 黑名单（命中即拦截）
    Interceptor::getInstance().registerTypeRule("uri_blacklist", {
        "/forbidden",
        "/hack.html",
        "/internal/debug",
        "/admin/deleteAll",
        "/api/hack", 
        "/shutdown"
    }, false); // false 表示黑名单机制
}