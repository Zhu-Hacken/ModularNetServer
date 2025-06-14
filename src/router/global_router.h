#pragma once

#include "router.h"

class GlobalRouter {
public:
    static Router& getInstance() {
        static Router instance;
        return instance;
    }    

    // 禁止拷贝/构造
    GlobalRouter() = delete;
    GlobalRouter(const GlobalRouter&) = delete;
    GlobalRouter& operator=(const GlobalRouter&) = delete;
};