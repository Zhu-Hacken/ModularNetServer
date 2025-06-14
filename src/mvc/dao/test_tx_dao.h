#pragma once

#include <string>

class TextTxDao {
public:
    static bool insertUser(std::string& username, int age);
};