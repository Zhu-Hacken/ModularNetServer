#pragma once
#include "conn/http/http_conn.h"
#include "conn/base_conn.h"
#include "timer/timer_manager.h"
#include "threadpool/thread_pool.h"
#include "util/utils.h"
#include "router/global_router.h"
#include "config/configs.h"
#include <netinet/in.h>
#include <unordered_map>
#include <memory>
#include <mutex>
/*
NetServer 类：负责管理监听 socket，事件循环，连接处理等服务端主控逻辑
*/ 
class NetServer
{
public:
    static NetServer& getInstance();

    ~NetServer();

    // 删除拷贝构造函数
    NetServer(const NetServer&) = delete;           
    NetServer& operator=(const NetServer&) = delete;
    
    // 初始化服务器
    void init(ServerConfig config, std::string& db_username, std::string& db_password, std::string& db_name, int db_port = 3306);
    // 启用服务器
    void run();           
    // 优雅关闭服务器
    void shutdown();

    // 获取连接对象
    std::shared_ptr<BaseConn> getConn(int fd);

private:
    NetServer();
    // NetServer(const ServerConfig& config);
    static const int MAX_EVENTS = 10000;

    // 配置&状态
    ServerConfig m_config;  // 配置参数
    bool m_is_running;  // 标识当前服务器是否正在运行
    bool m_shutdown_called; // 防止重复 shutdown
    int m_http_port;         // http监听端口
    

    // === fd管理 ===
    int m_listen_fd;    // 监听socket的文件描述符
    std::unordered_set<int> m_listen_fds;  // 监听socket的文件描述符集合
    int m_epoll_fd;     // epoll 实例的描述符
    int m_pipe_fd[2];   // 信号pipe，0为读端，1为写端

    // === 并发模型 ===
    int m_trig_mode;   // 触发模式：0 = LT（水平触发），1 = ET（边沿触发）
    int m_actor_model; // 并发模型：0 = Proactor，1 = Reactor

    // === 连接管理 ===
    std::unordered_map<int, std::shared_ptr<BaseConn>> m_users; // fd -> 连接对象 映射
    mutable std::mutex m_users_mtx;   // 新增：保护 m_users
    sockaddr_in m_address;  // 服务器地址结构
    std::unordered_map<int, int> m_listenFdToPort;  // fd -> 端口 映射
    std::unordered_map<int, sockaddr_in> m_listenFdToAddr;  // fd -> Addr 映射
    // === 核心模块 ===
    TimerManager m_timer_manager;   // 定时器
    // ThreadPool m_thread_pool;       // 线程池
    std::unique_ptr<ThreadPool> m_thread_pool;       // 线程池
    bool m_close_log;       // 日志

    // === 核心事件 ===
    // 初始化信号处理与优雅关闭
    void initSignalHandlers();
    // 创建 socket，绑定端口，listen，设置 socket 选项
    void initSocket();      
    // 初始化数据库连接池
    // void initSqlConnPool(std::string& db_username, std::string& db_password, std::string& db_name, int db_port = 3306);
    // 初始化epoll实例
    void initEpoll();
    // 处理连接事件
    void handleNewConnection(int listen_fd);
    // 处理信号事件
    void handleSignalEvent();
    // 处理读事件
    void handleReadEvent(int fd);
    // 处理写事件
    void handleWriteEvent(int fd);
    // 刷新连接对应的定时器
    void refreshTimer(int fd);
    // 关闭连接
    void closeConnection(int fd);
    // 注册事件
    void addFd(int fd, uint32_t events);
    void addFd(int fd, uint32_t events, bool one_shot, bool trig_mode);
    // 修改事件
    void modFd(int fd, uint32_t events);
    void modFd(int fd, uint32_t events, bool one_shot, bool trig_mode);
    // 移除事件
    void removeFd(int fd);
    // 获取全局路由
    inline Router& getRouter() {
        return GlobalRouter::getInstance();
    }
    // 初始化连接对象工厂
    // void initConnFactory();
    // 初始化热更新配置管理器
    // void initConfigManager();
    // === 主循环 ===
    void eventLoop();
};
