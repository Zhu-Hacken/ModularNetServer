#include "server_config.h"
#include <iostream>

ServerConfig::ServerConfig() : 
                        http_port(9006),
                        websocket_port(9007),
                        test_port(9999),
                        thread_num(8), 
                        conn_num(8), 
                        trig_mode(LT), 
                        actor_model(Proactor), 
                        log_close(true), 
                        rate_limiter_close(false),
                        interceptor_close(false),
                        config_manager_close(false)
{

}     // 初始化
ServerConfig::~ServerConfig(){}

void ServerConfig::parseArgs(int argc, char* argv[]){ // 解析命令行参数
    for(int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if(arg == "-p" && i + 1 < argc) http_port = std::stoi(argv[++i]);
        else if(arg == "-t" && i + 1 < argc) thread_num = std::stoi(argv[++i]);
        else if(arg == "-s" && i + 1 < argc) conn_num = std::stoi(argv[++i]);
        else if(arg == "-l" && i + 1 < argc) log_write = std::stoi(argv[++i]);
        else if(arg == "-m" && i + 1 < argc) trig_mode = std::stoi(argv[++i]);
        else if(arg == "-o" && i + 1 < argc) opt_linger = std::stoi(argv[++i]);
        else if(arg == "-c" && i + 1 < argc) log_close = std::stoi(argv[++i]);
        else if(arg == "-a" && i + 1 < argc) actor_model = std::stoi(argv[++i]);


    }
}
void ServerConfig::printConfig() const {
    
}
