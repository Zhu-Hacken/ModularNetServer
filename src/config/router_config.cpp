#include "router_config.h"
#include "mvc/controller/test_controller.h"
#include "mvc/controller/user_controller.h"
#include "mvc/controller/test_tx_controller.h"
// 在此处注册Controllers，即可实现路由跳转
void RouterConfig::registerAllRoutes() {
    TestController::registerRoutes();
    UserController::registerRoutes();
    TestTxController::registerRoutes();
}