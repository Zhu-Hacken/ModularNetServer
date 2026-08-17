// 管理监听 socket、accept、事件分发
#include "net_server.h"
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>
#include <signal.h>
#include "db/sql_connection_pool.h"
#include "conn/conn_factory_manager.h"
#include "log/logs.h"

// std::set<int> to_be_closed;  // NetServer 的成员变量

const std::string BASE_TEXT = "[NetServer] ";

NetServer& NetServer::getInstance() {
    static NetServer instance;
    return instance;
}

NetServer::NetServer()
  : m_is_running(true)
{
    LOG_INFO(BASE_TEXT + "NetServer created.");
}


NetServer::~NetServer(){
    if (!m_shutdown_called) {
         LOG_WARN(BASE_TEXT + "~NetServer() 自动调用 shutdown()");
        shutdown();
    }
}

void NetServer::init(ServerConfig config,
                     std::string& db_username, 
                     std::string& db_password, 
                     std::string& db_name, 
                     int db_port) 
{
    m_config = config;
    m_http_port = config.http_port;
    m_trig_mode = config.trig_mode; 
    // m_listen_fd(-1) ,
    m_actor_model = config.actor_model; 
    m_thread_pool = std::unique_ptr<ThreadPool>(new ThreadPool(config.thread_num));
    m_close_log = config.log_close;

    initSignalHandlers();
    // initSqlConnPool(db_username, db_password, db_name);
    initSocket();
    initEpoll();


    // === 路由器 ===
    getRouter().registerGet("/api/info", [this](HttpRequest& http_request, HttpResponse& http_response){
        Json data;
        data["http_port"] = m_http_port;
        data["thread_num"] = m_config.thread_num;
        data["trig_mode"] = (m_trig_mode == 1) ? "ET" : "LT";
        data["actor_model"] = (m_actor_model == 1) ? "Reactor" : "Proactor";

        Json response;
        response["data"] = data;
        response["message"] = "success";
        response["status"] = "200";

        http_response.sendJson(200, response);
    });

    
}

void NetServer::initSignalHandlers() {
    SignalUtils::init();
}

void NetServer::initSocket(){     // 创建 socket，绑定端口，listen，设置 socket 选项
    LOG_INFO(BASE_TEXT + "initSocket() called");
    std::vector<int> ports = {m_config.http_port, m_config.test_port, m_config.websocket_port};

    for (auto port : ports) {

        // 创建监听socket
        int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        m_listen_fds.insert(listen_fd);
        m_listenFdToPort.insert({listen_fd, port});
        // 设置端口复用
        int opt = 1;
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));


        sockaddr_in address;
        // 绑定端口
        address.sin_family = AF_INET;                     // 使用IPv4
        address.sin_addr.s_addr = htonl(INADDR_ANY);      // 监听本机所有网卡
        address.sin_port = htons(port);                 // 端口（大端）
        
        m_listenFdToAddr[listen_fd] = address;

        if(bind(listen_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
            LOG_ERROR(BASE_TEXT + "Error:bind failed");
            close(listen_fd);
            exit(EXIT_FAILURE);
        }

        // 开始监听
        if(listen(listen_fd, 128) < 0){
            LOG_ERROR(BASE_TEXT + "Error: listen failed");
            close(listen_fd);
            return;
        }
        LOG_INFO(BASE_TEXT + "Server listening on port " + std::to_string(port) + "...");
        // signal(SIGPIPE, SIG_IGN);
    }


    // 设置连接对象触发模式
    BaseConn::m_trig_mode = m_trig_mode;
    BaseConn::m_actor_model = m_actor_model;
}    

void NetServer::initEpoll() {
    // 创建epoll实例
    m_epoll_fd = epoll_create1(0);
    if(m_epoll_fd < 0) {
        LOG_ERROR(BASE_TEXT + "Error: epoll_create1 failed");
        exit(EXIT_FAILURE);
    }
    for ( int listen_fd : m_listen_fds) {
        // 注册监听socket到epoll
        addFd(listen_fd, EPOLLIN, false, false);
    }

    // 监听信号管道读端
    int signal_fd = SignalUtils::getSignalReadFd();
    addFd(signal_fd, EPOLLIN, false, false);

    LOG_INFO(BASE_TEXT + "epoll 初始化完成，epoll_fd = " + std::to_string(m_epoll_fd));
}


void NetServer::run() {
    LOG_INFO(BASE_TEXT + "NetServer::run() 启动");
    eventLoop();
}

void NetServer::eventLoop(){           // 启用服务器
    LOG_INFO(BASE_TEXT + "服务器主事件循环开始");
    // 3. 开始事件循环
    epoll_event events[MAX_EVENTS];
    while(m_is_running) {
        // 等待事件到达
        LOG_INFOF("%sEopll waiting.", BASE_TEXT.c_str());
        int event_count  = epoll_wait(m_epoll_fd, events, MAX_EVENTS, -1);
        if(event_count  < 0 && errno != EINTR){
            LOG_ERROR(BASE_TEXT + "Error: epoll_wait failed");
        }
        LOG_INFOF("%sEopll event_count  = %d" , BASE_TEXT.c_str(), event_count );
        
        // 检查过期定时器
        m_timer_manager.tick();

        // 对事件进行循环处理
        for (int i = 0; i < event_count ; i++) {
            int cur_fd = events[i].data.fd;
            if(m_listen_fds.count(cur_fd)) { 
                if (cur_fd < 0) {
                    LOG_ERROR(BASE_TEXT + "Event error: fd = " + std::to_string(cur_fd) + ", errno = " + std::to_string(errno));
                    continue;
                }
                // 连接事件处理 
                handleNewConnection(cur_fd);
                
            }
            else if (cur_fd == SignalUtils::getSignalReadFd()) {
                // 信号事件处理
                handleSignalEvent();
            }
            else {
                // 普通事件处理
                if(events[i].events & EPOLLIN) {
                    handleReadEvent(cur_fd);
                }
                else if(events[i].events & EPOLLOUT) {
                    handleWriteEvent(cur_fd);
                }
            }
        }
    }
    close(m_epoll_fd);
    LOG_INFO(BASE_TEXT + "服务器主循环退出，已关闭 epoll_fd");
}

void NetServer::shutdown() {
    LOG_WARN(BASE_TEXT + "shutdown()被调用，服务器即将优雅关闭...");


    for (int listen_fd : m_listen_fds) {
        // 关闭监听socket
        if (listen_fd >= 0) {
            close(listen_fd);
            listen_fd = -1;
        }
    }

    // 关闭所有用户连接
    {
        std::lock_guard<std::mutex> lock(m_users_mtx);
        for (std::unordered_map<int, std::shared_ptr<BaseConn>>::iterator it = m_users.begin(); it != m_users.end(); ++it) {
            if (it->second) {
                it->second->closeConn();
            }
        }
        m_users.clear();
    }

    // 销毁数据库连接池
    SqlConnPool::getInstance().shutdown();
    // 关闭线程池
    m_thread_pool->shutdown();
    // 关闭日志
    Log::getInstance().shutdown();

    // 结束主循环
    m_is_running = false;

    // 标记为已调用
    m_shutdown_called = true;

    // 写日志提示
    LOG_INFO(BASE_TEXT + "shutdown完成，退出成功");
}

void NetServer::handleNewConnection(int listen_fd)
{
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int conn_fd = accept(listen_fd, (sockaddr*)&client_addr, &client_len);
    if (conn_fd < 0) {
        LOG_ERROR(BASE_TEXT + "accept failed: fd = " + std::to_string(listen_fd) + ", errno = " + std::to_string(errno));
        return;
    }

    LOG_INFOF("%sNew connection accepted: fd = %d", BASE_TEXT.c_str(), conn_fd);

    // 注册 conn_fd 到 epoll
    addFd(conn_fd, EPOLLIN);
    int port = m_listenFdToPort[listen_fd];
    // auto conn = std::make_shared<HttpConn>();
    auto conn = ConnFactoryManager::getInstance().createConn(port);

    // 上锁插入
    {
        std::lock_guard<std::mutex> lock(m_users_mtx);
        m_users[conn_fd] = conn;
    }

    conn->init(conn_fd, client_addr);

    // std::cout << std::to_string(conn_fd) + ": before setEpollWriteCallback." << std::endl;

    std::weak_ptr<BaseConn> wp = conn;
    // conn->setEpollWriteCallback([this, conn](int fd) {
    conn->setEpollWriteCallback([this, wp](int fd) {
        // std::cout << std::to_string(fd) + ": modFd in setEpollWriteCallback." << std::endl;
        auto conn = wp.lock();
        if (!conn || conn->isClosed()) {
            LOG_ERROR(BASE_TEXT + "modFd in httpconn 被调用但连接已关闭或无效: fd = " + std::to_string(fd));
            return;
        }
        // std::cout << "before this->modFd(" << fd << ", EPOLLOUT);" << std::endl;
        this->modFd(fd, EPOLLOUT);
        // std::cout << "after this->modFd(" << fd << ", EPOLLOUT);" << std::endl;
    });

    refreshTimer(conn_fd);
}

void NetServer::handleSignalEvent() {
    int sig = 0;
    while (read(SignalUtils::getSignalReadFd(), &sig, 1) > 0) {
        LOG_WARN(BASE_TEXT + "收到信号：" + std::to_string(sig));
        switch (sig) {
            case SIGINT:
            case SIGTERM:
                LOG_WARN(BASE_TEXT + "收到终止信号，准备关闭服务器...");
                shutdown();     // 优雅关闭
                break;
            default:
                LOG_INFO(BASE_TEXT + "忽略信号：" + std::to_string(sig));
                break;
        }
    }
}

void NetServer::handleReadEvent(int fd)
{
    std::shared_ptr<BaseConn> conn;
    {
        std::lock_guard<std::mutex> lock(m_users_mtx);
        auto it = m_users.find(fd);
        if (it == m_users.end() || it->second->isClosed()) {
            LOG_ERROR(BASE_TEXT + "handleReadEvent 被调用但连接已关闭或无效: fd = " + std::to_string(fd));
            return;
        }
        conn = it->second;
    }
    if(m_actor_model) {    // Reactor
        // 交给线程池异步处理
        m_thread_pool->addTask([this, conn, fd]() {
            int n = conn->read();
            if(n > 0){
                bool success = conn->process();
                if (!success) {
                    LOG_INFOF("%sReactor数据未读完整，modFd.", BASE_TEXT.c_str());
                    this->modFd(fd, EPOLLIN);
                }
                if (!conn->isClosed()){     // 响应构建失败时会断开连接，若断开则不刷新定时器
                    refreshTimer(fd);
                }
            } 
            else if(n == 0){
                LOG_INFOF( "%sReactor 客户端主动关闭连接 fd = %d" , BASE_TEXT.c_str(), fd);
                closeConnection(fd);
            }
            else {

                LOG_ERROR(BASE_TEXT + "Reactor read failed, closing fd = " + std::to_string(fd));
                closeConnection(fd);
            }
        });
    } else {                // Proactor
        // 主线程负责 read，子线程负责业务处理
        int n = conn->read();
        if (n > 0) {
            m_thread_pool->addTask([this, conn, fd]() {
                bool success = conn->process();
                if(!success) {
                    LOG_INFOF( "%sProactor数据未读完整，modFd.", BASE_TEXT.c_str());
                    this->modFd(fd, EPOLLIN);
                }
                if (!conn->isClosed()) {
                    refreshTimer(fd);
                }
            });
        }
        else if (n == 0){
             
            LOG_INFOF( "%sProactor 客户端主动关闭连接 fd = %d", BASE_TEXT.c_str(), fd);
            closeConnection(fd);
        }
        else {
            LOG_ERROR(BASE_TEXT + "Proactor read failed, closing fd = " + std::to_string(fd));
            closeConnection(fd);
        }
    }
}

void NetServer::handleWriteEvent(int fd)
{
    std::shared_ptr<BaseConn> conn;
    {
        std::lock_guard<std::mutex> lock(m_users_mtx);
        auto it = m_users.find(fd);
        if (it == m_users.end() || it->second->isClosed()) {
            LOG_ERROR(BASE_TEXT + "handleWriteEvent 被调用但连接已关闭或无效: fd = " + std::to_string(fd));
            return;
        }
        conn = it->second;
    }
    if (m_actor_model) {    // Reactor
        m_thread_pool->addTask([this, conn, fd](){
            // if(conn->write()){
            if(conn->write() == WriteStatus::OK){
                if(conn->getKeepAlive()){ // 保持连接
                    conn->init(fd, conn->getAddr());
                    modFd(fd, EPOLLIN);
                    refreshTimer(fd);
                }
                else {
                    closeConnection(fd);    
                }
            } 
            else if (conn->write() == WriteStatus::AGAIN) {
                modFd(fd, EPOLLOUT);
                refreshTimer(fd);
            }
            else {
                closeConnection(fd);
            }
        });
    }
    else {                  // Proactor
        // if(conn->write()){
        m_thread_pool->addTask([this, conn, fd](){
            if(conn->write() == WriteStatus::OK){
                if(conn->getKeepAlive()) {    // 保持连接
                    conn->init(fd, conn->getAddr());
                    modFd(fd, EPOLLIN);
                    refreshTimer(fd);
                }
                else {
                    closeConnection(fd);    
                }
            }
            else if(conn->write() == WriteStatus::AGAIN) {
                modFd(fd, EPOLLOUT);
                refreshTimer(fd);
            }
            else {
                closeConnection(fd);
            }
        });
    }
}

void NetServer::refreshTimer(int fd)
{   
    LOG_INFOF("%s刷新定时器：fd = %d", BASE_TEXT.c_str(), fd);
    // 设置定时器
    m_timer_manager.addTimer(fd, [fd, this](){
        this->closeConnection(fd);
        LOG_INFOF("%s超时断开连接 fd = %d", BASE_TEXT.c_str(), fd);
        }, 1000000);
}

void NetServer::closeConnection(int fd)
{
    std::shared_ptr<BaseConn> conn;
    {
        std::lock_guard<std::mutex> lock(m_users_mtx);

        auto it = m_users.find(fd);

        // 如果连接已经关闭或无效，跳过
        if(it == m_users.end()) return;
        if(it->second->isClosed()) return;

        conn = it->second;
        // 连接对象销毁时由 shared_ptr 自动管理
        m_users.erase(it);
    }
     // 删除 epoll 事件，确保不会继续操作已关闭的 fd
    removeFd(fd);

    // 通过 shared_ptr 调用 closeConn，确保自动管理连接生命周期
    conn->closeConn();

    LOG_INFOF( "%sConnection closed: fd = %d", BASE_TEXT.c_str(), fd);
    // std::cout << (BASE_TEXT + "Connection closed: fd = " + std::to_string(fd)) << std::endl;
}

void NetServer::addFd(int fd, uint32_t events)
{
    addFd(fd, events, true, m_trig_mode);
}

void NetServer::addFd(int fd, uint32_t events, bool one_shot, bool trig_mode)
{
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = events;  

    if(one_shot) ev.events |= EPOLLONESHOT;
    if(trig_mode) ev.events |= EPOLLET;

    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        LOG_ERROR(BASE_TEXT + "Error: addFd failed: fd = " + std::to_string(fd));
        return;
    }

    // 设置非阻塞
    SysUtils::setNonBlocking(fd);
}

void NetServer::modFd(int fd, uint32_t events)
{
    // std::cout << "before modFd(" << fd << ", events, true, m_trig_mode);" << std::endl;
    modFd(fd, events, true, m_trig_mode);
    // std::cout << "after modFd(" << fd << ", events, true, m_trig_mode);" << std::endl;
}

void NetServer::modFd(int fd, uint32_t events, bool one_shot, bool trig_mode)
{
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = events;  

    if(one_shot) ev.events |= EPOLLONESHOT;
    if(trig_mode) ev.events |= EPOLLET;

    if(events == EPOLLIN){
        LOG_INFOF("%smodFd(EPOLLIN) called: fd = %d", BASE_TEXT.c_str(), fd);
    }
    if(events == EPOLLOUT){
        LOG_INFOF("%smodFd(EPOLLOUT) called: fd = %d", BASE_TEXT.c_str(), fd);
    }
    // std::cout << "before if (epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, fd, &ev) < 0) {" << std::endl;
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, fd, &ev) < 0) {
        // std::cout << BASE_TEXT + "after modFd error fd = " << fd << ".\n";
        LOG_ERROR(BASE_TEXT + "Error: modFd failed: fd = " + std::to_string(fd));
        return;
    }
    // std::cout << "after if (epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, fd, &ev) < 0) {" << std::endl;
}

void NetServer::removeFd(int fd)
{
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr) < 0) {
        LOG_ERROR(BASE_TEXT + "Error: removeFd failed: fd = " + std::to_string(fd));
        return;
    }
}


std::shared_ptr<BaseConn> NetServer::getConn(int fd) {
    std::lock_guard<std::mutex> lock(m_users_mtx);
    auto it = m_users.find(fd);
    if (it == m_users.end() || it->second->isClosed()) return nullptr;
    return it->second;
}