#pragma once
#include <string>

/*
 * UserModel：用户数据模型
 * 封装基本的用户操作，如登录验证、注册插入
 */

class UserDao {
public:
    // 校验用户名和密码是否正确
    static bool checkUser(const std::string& username, const std::string& password);
    // 插入新用户信息，返回是否成功
    static bool insertUser(const std::string& username, const std::string& password);
};