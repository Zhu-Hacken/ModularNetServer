#include "conn_factory_manager.h"

ConnFactoryManager& ConnFactoryManager::getInstance() {
    static ConnFactoryManager instance;
    return instance;
}

void ConnFactoryManager::registerFactory(int port, ConnFactory factory) {
    m_factoryMap[port] = factory;
}

std::shared_ptr<BaseConn> ConnFactoryManager::createConn(int port) {
    auto it = m_factoryMap.find(port);
    if (it != m_factoryMap.end()) {
        return it->second();
    }
    return nullptr;
}