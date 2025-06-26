#pragma once

/*
参数配置（端口、模式、目录）
*/
class ServerConfig{

public:
    enum TrigMode {
        LT,
        ET
    };

    enum ActorModel{
        Proactor,
        Reactor
    };
public:
    ServerConfig();     // 初始化
    ~ServerConfig();
    
    // 解析命令行参数
    void parseArgs(int argc, char* argv[]); 
    void printConfig() const;

public:

    int http_port;       // http端口
    int websocket_port;       // websocket端口
    int test_port;       // 测试端口（无意义）
    int thread_num; // 线程池数量
    int conn_num;    // 数据库连接池数量
    int log_write;  // 日志写入方式（目前只支持异步）
    int trig_mode;  // 触发模式
    int opt_linger; // 优雅关闭连接
    int log_close;  // 关闭日志
    int actor_model;    // 并发模型选择
    bool rate_limiter_close;  // 关闭频率限制器
    bool interceptor_close;  // 关闭拦截器
    bool config_manager_close;  // 关闭热更新
};