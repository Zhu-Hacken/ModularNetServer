#include "interceptor_config.h"

InterceptorConfig::InterceptorFunc InterceptorConfig::m_interceptor_func = nullptr;

void InterceptorConfig::setInterceptorFunc(InterceptorFunc func) {
    m_interceptor_func = func;
}

void InterceptorConfig::registerAllInterceptor() {
    if (m_interceptor_func) m_interceptor_func();

    
}