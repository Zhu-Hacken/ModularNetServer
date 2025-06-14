#pragma once
#include "server_config.h"
#include "router_config.h"
#include "rate_limiter_config.h"
#include "interceptor_config.h"
#include "config_manager.h"
/*
 * 配置模块清单（用于集中引入所有配置相关头文件）：
 *   - service_config.h     服务器运行配置（端口、模式、线程数等）
 *   - router_config.h      路由注册配置（集中注册所有 Controller 路由）
 *   - rate_limiter_config.h    频率限制器配置（提供 registerAllRateLimiter() 注册限制规则）
 *   - interceptor_config.h     拦截器配置（提供 registerAllInterceptor() 注册拦截规则）
 *   - config_manager.h         配置管理器配置（热更新）
 */