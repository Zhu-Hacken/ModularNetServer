#include "net/net_server.h"
#include <cstring>
#include <cassert>
#include "util/utils.h"
#include "config/configs.h"
#include "log/logs.h"
#include "init.h"

const std::string BASE_TEXT = "[Main] ";

int main(int argc, char* argv[]){

    std::string username = "webuser";
    std::string password = "webpwd";
    std::string databasename = "littlewebserver";

    // 从命令行解析配置
    ServerConfig config;
    config.parseArgs(argc, argv);

    // 初始化所有单例模块
    initAllModules(config, username, password, databasename);


    LOG_INFO(BASE_TEXT + "========== ModularNetServer启动 ==========");

    // 打印当前配置模式
    std::string actor_str = (config.actor_model == ServerConfig::Proactor) ? "Proactor" : "Reactor";
    std::string trig_str = (config.trig_mode == ServerConfig::ET) ? "ET" : "LT";

    std::cout << "[ServerConfig] 当前模式："
              << actor_str << " + " << trig_str << std::endl;


    NetServer::getInstance().init(config, username, password, databasename);
    NetServer::getInstance().run();
    std::cout << BASE_TEXT + "服务器已关闭。" << std::endl;
    return 0;
}