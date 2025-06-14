#include "user_service.h"
#include "mvc/dao/user_dao.h"

// 登陆校验
bool UserService::login(const std::string& username, const std::string& password) {
    return UserDao::checkUser(username, password);
}

// 注册新用户
bool UserService::registerUser(const std::string& username, const std::string& password) {
    return UserDao::insertUser(username, password);
}