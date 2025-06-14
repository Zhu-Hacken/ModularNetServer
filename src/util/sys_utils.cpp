#include "sys_utils.h"
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <experimental/filesystem>

SysUtils::~SysUtils()
{
}

int SysUtils::setNonBlocking(int fd){
    int oldFlags = fcntl(fd, F_GETFL, 0);
    if (oldFlags < 0) {
            std::cerr << "[SysUtil] SetNonBlocking fcntl(F_GETFL) failed\n";
            return -1;
        }

        if (fcntl(fd, F_SETFL, oldFlags | O_NONBLOCK) < 0) {
            std::cerr << "[SysUtil] SetNonBlocking fcntl(F_SETFL) failed\n";
            return -1;
        }

    return oldFlags;
}

int SysUtils::setReuseAddr(int fd){
    return 0;
}

std::string SysUtils::getThreadIdStr()
{
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return oss.str();
}

std::string SysUtils::getRootPath(){
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count != -1) {
        std::string path(result, count);
        std::cout << std::experimental::filesystem::path(path).string() <<std::endl;
        return std::experimental::filesystem::path(path).parent_path().parent_path().string();  // 返回项目根目录
    }
    return "./";  // 出错时默认返回当前目录
}
