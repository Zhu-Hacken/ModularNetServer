#include <iostream>
#include "net/web_server.h"
#include <cstring>
#include <cassert>
#include "util/utils.h"
#include "config/configs.h"

const std::string BASE_TEXT = "[Main] ";


void initConfigManager() {
    ConfigManager::getInstace().init(true);
    LOG_INFO(BASE_TEXT + ConfigManager::getInstace().get("log_close"));

    ConfigManager::getInstace().registerCallback("log_close", [](){
        bool enabled = !ConfigManager::getInstace().getBool("log_close", false);
        LOG_INFO("enabled == " + std::to_string(enabled));
    });
}


int main(int argc, char* argv[]){

    std::string username = "webuser";
    std::string password = "webpwd";
    std::string databasename = "littlewebserver";
    // 从命令行解析配置
    ServerConfig config;
    config.parseArgs(argc, argv);

    // === 日志模块 ===
    Log::getInstance().init(Log::DEBUG, "server_log", !config.log_close);

    // === ConfigManager（热更新）===
    initConfigManager();

    LOG_INFO(BASE_TEXT + "========== LittleWebServer启动 ==========");
    WebServer server(config);
    

    server.init(username, password, databasename);
    server.run();
    std::cout << BASE_TEXT + "服务器已关闭。" << std::endl;
    return 0;
}