#pragma once

#include <string>

class UserService {
public:
    // 登陆校验
    static bool login(const std::string& username, const std::string& password);

    // 注册新用户
    static bool registerUser(const std::string& username, const std::string& password);


private:

};