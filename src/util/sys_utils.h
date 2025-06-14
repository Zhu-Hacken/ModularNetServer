#pragma once
#include <sstream>
#include <thread>

#define PATH_MAX 4096

/*
工具类
*/
class SysUtils
{
public:
    SysUtils() = delete;
    ~SysUtils();

    // 设置非阻塞
    static int setNonBlocking(int fd);  
    // 设置端口复用
    static int setReuseAddr(int fd);    
    // 获取当前线程id
    static std::string getThreadIdStr();
    // 获取项目根路径
    static std::string getRootPath();  

private:
};

