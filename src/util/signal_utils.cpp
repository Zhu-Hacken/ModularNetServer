#include "signal_utils.h"
#include "util/utils.h"
#include <cstring>
#include <cassert>
#include "log/logs.h"


const std::string BASE_TEXT = "[SignalUtils] ";

int SignalUtils::m_pipe_fd[2] = {-1, -1};

void SignalUtils::init() {
    initSignalPipe();

    registerSignal(SIGPIPE, SIG_IGN);
    registerSignal(SIGINT, handleSignal);
    registerSignal(SIGTERM, handleSignal);

    LOG_INFO(BASE_TEXT + "信号机制初始化完成");   
}

int SignalUtils::getSignalReadFd() {
    return m_pipe_fd[0];
}

void SignalUtils::registerSignal(int sig, void (*handler)(int), bool restart) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));     // 初始化结构体
    sa.sa_handler = handler;                // 设置处理函数

    if (restart) {
        sa.sa_flags |= SA_RESTART;          // 系统调用被信号中断后自动重启
    }

    sigfillset(&sa.sa_mask);            // 在处理过程中阻塞其他所有信号
    int ret = sigaction(sig, &sa, nullptr); // 注册信号
    assert(ret != -1);                      // 调试断言，注册失败直接中止
}

void SignalUtils::initSignalPipe() {
    // 创建pipe
    if (pipe(m_pipe_fd) == -1) {
        LOG_ERROR(BASE_TEXT + "创建pipe失败");
        exit(EXIT_FAILURE);
    }

    //设置非阻塞
    SysUtils::setNonBlocking(m_pipe_fd[0]);
    SysUtils::setNonBlocking(m_pipe_fd[1]);

    LOG_INFO(BASE_TEXT + "信号管道创建成功，fd[0] = " + std::to_string(m_pipe_fd[0]) + ", fd[1] = " + std::to_string(m_pipe_fd[1]));
}

void SignalUtils::handleSignal(int sig) {
    LOG_WARN(BASE_TEXT + "捕获信号：" + std::to_string(sig));

    int save_errno = errno; // 保存原错误玛

    int msg = sig;
    ssize_t n = write(m_pipe_fd[1], (char*)&msg, 1);    // 写一个字节表示信号
    (void)n;    // 避免编译器警告

    errno = save_errno;
}