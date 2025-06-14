#pragma once

class RouterConfig {
public:
    // 实现函数指针 Hook 模式
    using RouteRegisterFunc = void(*)();
    static void setRouteRegisterFunc(RouteRegisterFunc func);   // 函数指针注入控制器注册逻辑
    static void registerAllRoutes();
private:
    static RouteRegisterFunc m_register_func;
};