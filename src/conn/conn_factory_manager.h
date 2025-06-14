#pragma once

#include <functional>
#include <memory>
#include "base_conn.h"
#include <unordered_map>

class ConnFactoryManager {
public:
    using ConnFactory = std::function<std::shared_ptr<BaseConn>()>;

    static ConnFactoryManager& getInstance();

    void registerFactory(int port, ConnFactory factory);
    std::shared_ptr<BaseConn> createConn(int port);

private:
    std::unordered_map<int, ConnFactory> m_factoryMap;

};