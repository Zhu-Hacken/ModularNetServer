#pragma once
#include "util/interceptor.h"

class InterceptorConfig {
public:
    using InterceptorFunc = void(*)();
    static void setInterceptorFunc(InterceptorFunc func);   
    static void registerAllInterceptor();
private:
    static InterceptorFunc m_interceptor_func;
};