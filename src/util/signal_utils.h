#pragma once

#include <signal.h>
#include <functional>

class SignalUtils {
public:
    // 初始化信号处理机制：设置管道 + 注册信号
    static void init();
    // 返回信号管道的读端
    static int getSignalReadFd();
    // 注册信号处理函数（可选开启重启）
    static void registerSignal(int sig, void (*handler)(int), bool restart = true);
    // 初始化常用信号（例如SIGPIPE，SIGINT，SIGTERM）

private:
    // 初始化管道pipe
    static void initSignalPipe();
    // 统一信号处理函数
    static void handleSignal(int sig);          

private:
    static int m_pipe_fd[2];                    // 管道
};