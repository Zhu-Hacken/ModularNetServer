#include "router_config.h"


// RouteRegisterFunc RouterConfig::m_register_func = nullptr;
RouterConfig::RouteRegisterFunc RouterConfig::m_register_func = nullptr;

void RouterConfig::setRouteRegisterFunc(RouteRegisterFunc func) {
    m_register_func = func;
}

// 在此处注册Controllers，即可实现路由跳转
void RouterConfig::registerAllRoutes() {
    if (m_register_func) m_register_func();
    // TestController::registerRoutes();
    // UserController::registerRoutes();
    // TestTxController::registerRoutes();
}