# ModularNetServer

> 一个基于 C++11 的模块化、高性能网络服务器框架，支持 HTTP / WebSocket / TCP 多协议，内置线程池、定时器、路由、MVC、拦截器、限流、缓存、数据库连接池与事务、Session/Token 会话等组件。

---

## 目录

- [项目简介](#项目简介)
- [特性](#特性)
- [整体架构](#整体架构)
- [目录结构](#目录结构)
- [核心模块说明](#核心模块说明)
- [依赖环境](#依赖环境)
- [编译与运行](#编译与运行)
- [命令行参数](#命令行参数)
- [配置说明](#配置说明)
- [示例接口](#示例接口)
- [设计亮点](#设计亮点)

---

## 项目简介

ModularNetServer 是一个从零实现的模块化网络服务器，强调「高内聚、低耦合」的分层与模块化设计。它以 Linux `epoll` 为核心事件驱动引擎，通过**连接工厂**支持多协议接入（HTTP / WebSocket / TCP），通过**路由 + MVC** 三层架构组织业务逻辑，并提供了线程池、定时器、拦截器、限流、缓存、数据库连接池、会话/令牌管理等一整套可复用基础设施组件。

项目采用 C++11 标准编写，所有核心模块均为单例或可插拔设计，可作为学习高性能服务器开发的范例，也可作为后续业务服务的基础框架。

---

## 特性

- **多并发模型**：支持 Reactor / Proactor 两种并发模型切换。
- **多触发模式**：支持水平触发（LT）/ 边沿触发（ET），并配合 `EPOLLONESHOT`。
- **多协议接入**：通过连接工厂 `ConnFactoryManager` 按端口分发，支持 HTTP、WebSocket、TCP。
- **多端口监听**：默认监听 HTTP（9006）、WebSocket（9007）、测试（9999）三个端口。
- **路由系统**：支持 GET / POST / WebSocket 路由注册与分发。
- **MVC 三层架构**：Controller → Service → DAO 分层，职责清晰。
- **线程池**：基于 `std::thread` + 条件变量的通用任务线程池。
- **定时器**：基于小根堆的定时器管理器，支持一次性/重复定时任务。
- **拦截器**：按「类型」维度配置白名单/黑名单，如免登录列表、URI 黑名单。
- **频率限制**：滑动窗口限流，可针对 IP、登录等维度防刷、防爆破。
- **缓存**：支持 LRU（内存）、Redis、以及 LRU + Redis 混合三种模式。
- **数据库**：MySQL 连接池 + 预处理语句（`SqlExecutor`）+ RAII 事务（`SqlTransaction`）。
- **会话与令牌**：Session 管理与基于 HMAC-SHA256 的 Token 认证。
- **配置热更新**：JSON 配置 + 后台监视线程，支持关键配置项动态变更。
- **优雅关闭**：通过信号管道（`pipe`）捕获 `SIGINT/SIGTERM` 优雅退出。
- **异步日志**：分级日志系统（DEBUG/INFO/WARN/ERROR/FATAL）。

---

## 整体架构

```
                      ┌─────────────────────────────────────────────┐
                      │                NetServer (单例)              │
                      │   epoll 事件循环 / 多端口监听 / 连接管理      │
                      └───────────────┬─────────────────────────────┘
                                      │ accept
                                      ▼
                      ┌─────────────────────────────────────────────┐
                      │         ConnFactoryManager（连接工厂）       │
                      │   port → HttpConn / WebSocketConn / TcpConn  │
                      └───────────────┬─────────────────────────────┘
                                      │
            ┌─────────────────────────┼─────────────────────────┐
            ▼                         ▼                         ▼
       HttpConn                 WebSocketConn               TcpConn
            │                         │
            ▼                         ▼
   ┌────────────────┐        ┌────────────────┐
   │   Router (全局) │        │ WebsocketConn  │
   │  GET/POST/WS    │        │ Manager (会话) │
   └───────┬────────┘        └────────────────┘
           │
           ▼
   Controller → Service → DAO
           │                │
           ▼                ▼
   ┌────────────┐    ┌──────────────┐
   │  Cache     │    │  SqlConnPool  │
   │ LRU/Redis  │    │  + Transaction│
   └────────────┘    └──────────────┘

   横切基础设施：ThreadPool / TimerManager / Interceptor / RateLimiter
                / SessionManager / TokenManager / ConfigManager / Log
```

---

## 目录结构

```
ModularNetServer-master/
├── CMakeLists.txt                 # 构建脚本
└── src/
    ├── main.cpp                   # 程序入口
    ├── init.{h,cpp}               # 各单例模块统一初始化
    ├── net/                       # 网络主控（NetServer、事件循环）
    │   └── net_server.{h,cpp}
    ├── conn/                      # 连接抽象与多协议实现
    │   ├── base_conn.{h,cpp}      # 连接基类（抽象）
    │   ├── conn_factory_manager.* # 端口 → 连接工厂
    │   ├── http/                  # HTTP 协议（解析/响应/请求）
    │   ├── tcp/                   # TCP 连接
    │   └── websocket/             # WebSocket 协议（握手/数据帧/连接管理）
    ├── router/                    # 路由系统（Router + GlobalRouter）
    ├── mvc/                       # 三层业务架构
    │   ├── controller/            # 控制器（Test / User / TestTx）
    │   ├── service/               # 服务层
    │   └── dao/                   # 数据访问层
    ├── threadpool/                # 线程池
    ├── timer/                     # 定时器（TimerNode + TimerManager）
    ├── config/                    # 配置（命令行 / 路由 / 限流 / 拦截器 / 热更新）
    ├── util/                      # 工具（拦截器 / 限流 / 加密 / 信号 / 系统）
    ├── db/                        # 数据库（连接池 / 预处理 / 事务 / RAII）
    ├── cache/                     # 缓存（LRU / Redis / 适配器）
    ├── session/                   # 会话管理
    ├── token/                     # 令牌认证
    ├── log/                       # 异步日志
    ├── third_party/nlohmann/      # JSON 库（内嵌）
    └── www/                       # 静态资源与示例页面
```

---

## 核心模块说明

### 1. 网络主控 `net/NetServer`

- 单例模式，负责监听 socket、`accept`、事件循环与连接生命周期管理。
- 基于 `epoll` 实现，支持多端口监听与信号管道监听。
- 提供 `init()` / `run()` / `shutdown()` 三段式生命周期，支持优雅关闭。
- 通过静态成员 `BaseConn::m_trig_mode`、`m_actor_model` 将触发模式与并发模型注入到每个连接对象。

### 2. 连接层 `conn/`

- `BaseConn`：连接抽象基类，定义 `init/read/write/process/closeConn` 等纯虚接口。
- `ConnFactoryManager`：维护「端口 → 连接工厂」映射，`accept` 后按端口创建对应连接对象，实现多协议接入。
- `HttpConn`：基于状态机（请求行 → 头部 → 请求体）的 HTTP 解析，支持静态文件、Range 分段、Cookie、Keep-Alive。
- `WebSocketConn`：实现 HTTP 握手升级、`Sec-WebSocket-Accept` 计算、数据帧解析，并配合 `WebsocketConnManager` 维护 fd 与 session 的双向映射。
- `TcpConn`：基础 TCP 连接。

### 3. 路由与 MVC `router/` + `mvc/`

- `Router` 维护 GET/POST/WebSocket 三张「路径 → 回调」映射表；`GlobalRouter` 提供全局单例。
- 业务按 **Controller → Service → DAO** 三层组织：
  - `UserController`：登录、注册、令牌校验。
  - `TestController`：`/api/hello`、`/api/time`、`/api/echo` 示例。
  - `TestTxController`：数据库事务示例。
  - DAO 层通过 `SqlExecutor` 预处理与 `SqlTransaction` RAII 事务访问数据库。

### 4. 并发与定时

- `ThreadPool`：任务队列 + 条件变量，通用线程池。
- `TimerManager`：基于 `priority_queue`（小根堆）的定时器，支持重复任务，用于连接超时、Session/Token 过期清理。

### 5. 安全与防护 `util/`

- `Interceptor`：按「类型」维度注册白名单（如免登录路径）或黑名单（如危险 URI）。
- `RateLimiter`：固定窗口限流，按「类型 + key」（如 `access_ip`、`login_ip`）统计请求次数。
- `CryptoUtils`：HMAC-SHA256、SHA1、Base64，用于 Token 签名与 WebSocket 握手。

### 6. 数据与缓存

- `SqlConnPool`：MySQL 连接池，阻塞获取/释放。
- `SqlExecutor`：封装 `MYSQL_STMT` 预处理语句，支持参数绑定，防止 SQL 注入。
- `SqlTransaction`：RAII 事务，构造时 `begin`，析构时未提交自动 `rollback`。
- `CacheAdapter`：统一缓存入口，支持 `LRU_ONLY` / `REDIS_ONLY` / `LRU_REDIS` 三种模式。
- `LRUCache`：基于链表 + 哈希表的线程安全 LRU 实现。
- `RedisClient`：基于 hiredis 的 Redis 客户端封装。

### 7. 会话与认证

- `SessionManager`：Session 创建/校验/刷新/过期清理，配合定时器自动回收。
- `TokenManager`：生成带过期时间的 Token，映射 Token → 用户名，同样支持定时清理。

### 8. 配置与日志

- `ConfigManager`：加载 JSON 配置，启动后台线程定时检测文件变化并触发回调，实现热更新。
- `Log`：分级异步日志系统。

---

## 依赖环境

| 依赖          | 说明                                                         |
| ------------- | ------------------------------------------------------------ |
| CMake         | ≥ 3.10                                                       |
| 编译器        | 支持 C++11（g++/clang++）                                    |
| 操作系统      | Linux（依赖 `epoll`、`sys/socket.h`、`netinet/in.h`、`pthread`） |
| MySQL         | libmysqlclient（`mysql/mysql.h`）                            |
| Redis         | hiredis（`hiredis/hiredis.h`）                               |
| OpenSSL       | libssl、libcrypto                                            |
| nlohmann/json | 已内嵌于 `src/third_party/`                                  |

> 注意：本项目依赖 Linux 专属的系统调用（如 `epoll`），无法直接在 Windows 上编译运行。

---

## 编译与运行

```bash
# 1. 进入项目根目录，创建构建目录
mkdir -p build && cd build

# 2. 生成构建文件
cmake ..

# 3. 编译
make -j$(nproc)

# 4. 运行服务器
./ModularNetServerApp
```

默认配置下，服务器会监听：

- HTTP 端口：`9006`
- WebSocket 端口：`9007`
- 测试端口：`9999`

访问 `http://localhost:9006/` 即可看到欢迎页。

---

## 命令行参数

| 参数 | 含义                              | 默认值          |
| ---- | --------------------------------- | --------------- |
| `-p` | HTTP 端口                         | `9006`          |
| `-t` | 线程池线程数                      | `4`             |
| `-s` | 数据库连接池大小                  | `4`             |
| `-l` | 日志写入方式                      | `0`             |
| `-m` | 触发模式（0=LT，1=ET）            | `0`（LT）       |
| `-o` | 优雅关闭（opt_linger）            | `0`             |
| `-c` | 关闭日志                          | `0`             |
| `-a` | 并发模型（0=Proactor，1=Reactor） | `0`（Proactor） |

示例：

```bash
./ModularNetServerApp -p 8080 -t 8 -m 1 -a 1
```

---

## 配置说明

`src/config/server_config.json` 提供运行时的热更新配置项：

```json
{
    "log_level": 0,
    "log_close": true,
    "rate_limiter_close": false,
    "interceptor_close": false,
    "config_manager_close": true
}
```

- `log_level`：日志等级（0=DEBUG, 1=INFO, 2=WARN, 3=ERROR, 4=FATAL）
- `log_close`：是否关闭日志输出
- `rate_limiter_close`：是否关闭频率限制器
- `interceptor_close`：是否关闭拦截器
- `config_manager_close`：是否关闭热更新

---

## 示例接口

| 方法 | 路径              | 说明               |
| ---- | ----------------- | ------------------ |
| GET  | `/` `/index.html` | 首页               |
| GET  | `/login.html`     | 登录页             |
| GET  | `/api/hello`      | 返回 JSON Hello    |
| GET  | `/api/time`       | 返回当前时间       |
| GET  | `/api/info`       | 返回服务器运行信息 |
| GET  | `/checktoken`     | 校验 Token         |
| POST | `/api/echo`       | 回显请求体         |
| POST | `/login`          | 用户登录           |
| POST | `/register`       | 用户注册           |
| POST | `/api/tx_test`    | 数据库事务测试     |
| WS   | `/ws_test.html`   | WebSocket 测试页   |

---

## 设计亮点

1. **多协议可插拔**：`ConnFactoryManager` 按端口分发连接类型，新增协议只需实现 `BaseConn` 并注册工厂。
2. **并发模型与触发模式解耦**：通过静态成员 + 配置注入，Reactor/Proactor 与 LT/ET 可任意组合。
3. **MVC 分层清晰**：Controller 负责路由与参数，Service 负责业务，DAO 负责数据访问，职责单一。
4. **安全防护内置**：拦截器 + 限流 + 预处理语句 + HMAC-SHA256 Token 认证，从接入层到数据层均有防护。
5. **资源生命周期安全**：连接用 `shared_ptr` 管理，数据库连接/事务用 RAII 管理，定时器自动回收 Session/Token。
6. **配置热更新**：修改 JSON 配置无需重启即可生效，适合生产环境调优。

---

# ModularNetServer

> A modular, high-performance network server framework written in C++11, supporting HTTP / WebSocket / TCP multi-protocol, with built-in thread pool, timers, routing, MVC, interceptors, rate limiting, cache, database connection pool & transactions, and Session/Token management.

## Table of Contents

- [Introduction](#introduction)
- [Features](#features)
- [Architecture](#architecture)
- [Directory Layout](#directory-layout)
- [Core Modules](#core-modules)
- [Dependencies](#dependencies)
- [Build & Run](#build--run)
- [Command-line Arguments](#command-line-arguments)
- [Configuration](#configuration)
- [Sample Endpoints](#sample-endpoints)
- [Design Highlights](#design-highlights)

---

## Introduction

ModularNetServer is a modular network server built from scratch with an emphasis on high cohesion and low coupling. It uses Linux `epoll` as its core event-driven engine, supports multi-protocol access (HTTP / WebSocket / TCP) via a connection factory, organizes business logic through a Router + MVC three-tier architecture, and provides a full suite of reusable infrastructure components: thread pool, timers, interceptors, rate limiting, cache, database connection pool, and session/token management.

The project is written in C++11. All core modules are singletons or pluggable designs, making it both an excellent learning reference for high-performance server development and a foundation for real services.

---

## Features

- **Multiple concurrency models**: switchable between Reactor and Proactor.
- **Multiple trigger modes**: Level-Triggered (LT) / Edge-Triggered (ET), combined with `EPOLLONESHOT`.
- **Multi-protocol access**: dispatch by port via `ConnFactoryManager` for HTTP, WebSocket, and TCP.
- **Multi-port listening**: listens on HTTP (9006), WebSocket (9007), and test (9999) by default.
- **Routing system**: GET / POST / WebSocket route registration and dispatch.
- **MVC three-tier architecture**: Controller → Service → DAO with clear separation of concerns.
- **Thread pool**: generic task thread pool based on `std::thread` + condition variables.
- **Timers**: min-heap based timer manager supporting one-shot and repeating tasks.
- **Interceptor**: type-based whitelist/blacklist rules (e.g., login-free paths, URI blacklist).
- **Rate limiting**: sliding-window rate limiting per type + key (e.g., IP, login).
- **Cache**: LRU (in-memory), Redis, and LRU + Redis hybrid modes.
- **Database**: MySQL connection pool + prepared statements (`SqlExecutor`) + RAII transactions (`SqlTransaction`).
- **Session & Token**: session management and HMAC-SHA256 token authentication.
- **Config hot-reload**: JSON config with a background watch thread for dynamic updates.
- **Graceful shutdown**: captures `SIGINT/SIGTERM` via a signal pipe for clean exit.
- **Async logging**: leveled logging (DEBUG/INFO/WARN/ERROR/FATAL).

---

## Architecture

```
                      ┌─────────────────────────────────────────────┐
                      │              NetServer (singleton)          │
                      │   epoll event loop / multi-port / conn mgmt │
                      └───────────────┬─────────────────────────────┘
                                      │ accept
                                      ▼
                      ┌─────────────────────────────────────────────┐
                      │         ConnFactoryManager (factory)        │
                      │   port → HttpConn / WebSocketConn / TcpConn  │
                      └───────────────┬─────────────────────────────┘
                                      │
            ┌─────────────────────────┼─────────────────────────┐
            ▼                         ▼                         ▼
       HttpConn                 WebSocketConn               TcpConn
            │                         │
            ▼                         ▼
   ┌────────────────┐        ┌────────────────┐
   │   Router (global)│       │ WebsocketConn  │
   │  GET/POST/WS    │        │ Manager (sess.)│
   └───────┬────────┘        └────────────────┘
           │
           ▼
   Controller → Service → DAO
           │                │
           ▼                ▼
   ┌────────────┐    ┌──────────────┐
   │  Cache     │    │  SqlConnPool  │
   │ LRU/Redis  │    │  + Transaction│
   └────────────┘    └──────────────┘

   Cross-cutting: ThreadPool / TimerManager / Interceptor / RateLimiter
                  / SessionManager / TokenManager / ConfigManager / Log
```

---

## Directory Layout

```
ModularNetServer-master/
├── CMakeLists.txt                 # build script
└── src/
    ├── main.cpp                   # entry point
    ├── init.{h,cpp}               # unified singleton initialization
    ├── net/                       # server core (NetServer, event loop)
    │   └── net_server.{h,cpp}
    ├── conn/                      # connection abstraction & protocols
    │   ├── base_conn.{h,cpp}      # abstract connection base class
    │   ├── conn_factory_manager.* # port → connection factory
    │   ├── http/                  # HTTP protocol (parse/request/response)
    │   ├── tcp/                   # TCP connection
    │   └── websocket/             # WebSocket (handshake/frame/manager)
    ├── router/                    # routing (Router + GlobalRouter)
    ├── mvc/                       # three-tier business architecture
    │   ├── controller/            # controllers (Test / User / TestTx)
    │   ├── service/               # service layer
    │   └── dao/                   # data access layer
    ├── threadpool/                # thread pool
    ├── timer/                     # timers (TimerNode + TimerManager)
    ├── config/                    # config (CLI / router / limiter / interceptor / hot-reload)
    ├── util/                      # utils (interceptor / limiter / crypto / signal / sys)
    ├── db/                        # database (pool / prepared / transaction / RAII)
    ├── cache/                     # cache (LRU / Redis / adapter)
    ├── session/                   # session management
    ├── token/                     # token authentication
    ├── log/                       # async logging
    ├── third_party/nlohmann/      # bundled JSON library
    └── www/                       # static assets and demo pages
```

---

## Core Modules

### 1. Server core `net/NetServer`

- Singleton managing listening sockets, `accept`, the event loop, and connection lifecycle.
- Built on `epoll`, supporting multi-port listening and a signal pipe.
- Three-phase lifecycle: `init()` / `run()` / `shutdown()`, with graceful shutdown.
- Injects trigger mode and concurrency model into each connection via static members `BaseConn::m_trig_mode` / `m_actor_model`.

### 2. Connection layer `conn/`

- `BaseConn`: abstract connection base class defining pure virtual `init/read/write/process/closeConn`.
- `ConnFactoryManager`: maintains a "port → connection factory" map; after `accept`, creates the appropriate connection object by port for multi-protocol access.
- `HttpConn`: state-machine based HTTP parsing (request line → headers → body), with static files, Range segments, cookies, and Keep-Alive.
- `WebSocketConn`: implements HTTP upgrade handshake, `Sec-WebSocket-Accept` computation, and data frame parsing, working with `WebsocketConnManager` to maintain fd ↔ session mappings.
- `TcpConn`: basic TCP connection.

### 3. Routing & MVC `router/` + `mvc/`

- `Router` maintains three "path → callback" maps for GET/POST/WebSocket; `GlobalRouter` provides a global singleton.
- Business logic follows a **Controller → Service → DAO** three-tier structure:
  - `UserController`: login, register, token verification.
  - `TestController`: `/api/hello`, `/api/time`, `/api/echo` examples.
  - `TestTxController`: database transaction example.
  - DAO layer accesses the database through `SqlExecutor` prepared statements and `SqlTransaction` RAII transactions.

### 4. Concurrency & timers

- `ThreadPool`: task queue + condition variables, a generic thread pool.
- `TimerManager`: `priority_queue` (min-heap) based timer supporting repeat tasks, used for connection timeouts and Session/Token expiry cleanup.

### 5. Security `util/`

- `Interceptor`: type-based whitelist (e.g., login-free paths) or blacklist (e.g., dangerous URIs).
- `RateLimiter`: fixed-window rate limiting per "type + key" (e.g., `access_ip`, `login_ip`).
- `CryptoUtils`: HMAC-SHA256, SHA1, Base64 for token signing and WebSocket handshakes.

### 6. Data & cache

- `SqlConnPool`: MySQL connection pool with blocking get/release.
- `SqlExecutor`: wraps `MYSQL_STMT` prepared statements with parameter binding to prevent SQL injection.
- `SqlTransaction`: RAII transaction — `begin` on construction, auto `rollback` on destruction if not committed.
- `CacheAdapter`: unified cache entry supporting `LRU_ONLY` / `REDIS_ONLY` / `LRU_REDIS` modes.
- `LRUCache`: thread-safe LRU implemented with a linked list + hash map.
- `RedisClient`: Redis client wrapper based on hiredis.

### 7. Session & authentication

- `SessionManager`: create/validate/refresh/expire sessions with automatic timer cleanup.
- `TokenManager`: generates expiring tokens mapping token → username, also with timer cleanup.

### 8. Config & logging

- `ConfigManager`: loads JSON config, starts a background thread to detect file changes and trigger callbacks for hot-reload.
- `Log`: leveled async logging system.

---

## Dependencies

| Dependency    | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| CMake         | ≥ 3.10                                                       |
| Compiler      | C++11-capable (g++/clang++)                                  |
| OS            | Linux (requires `epoll`, `sys/socket.h`, `netinet/in.h`, `pthread`) |
| MySQL         | libmysqlclient (`mysql/mysql.h`)                             |
| Redis         | hiredis (`hiredis/hiredis.h`)                                |
| OpenSSL       | libssl, libcrypto                                            |
| nlohmann/json | bundled in `src/third_party/`                                |

> Note: this project depends on Linux-specific syscalls (e.g., `epoll`) and cannot be built/run directly on Windows.

---

## Build & Run

```bash
# 1. Enter the project root and create a build directory
mkdir -p build && cd build

# 2. Generate build files
cmake ..

# 3. Build
make -j$(nproc)

# 4. Run the server
./ModularNetServerApp
```

By default the server listens on:

- HTTP: `9006`
- WebSocket: `9007`
- Test: `9999`

Visit `http://localhost:9006/` for the welcome page.

---

## Command-line Arguments

| Flag | Meaning                                   | Default        |
| ---- | ----------------------------------------- | -------------- |
| `-p` | HTTP port                                 | `9006`         |
| `-t` | thread pool size                          | `4`            |
| `-s` | DB connection pool size                   | `4`            |
| `-l` | log write mode                            | `0`            |
| `-m` | trigger mode (0=LT, 1=ET)                 | `0` (LT)       |
| `-o` | graceful close (opt_linger)               | `0`            |
| `-c` | disable logging                           | `0`            |
| `-a` | concurrency model (0=Proactor, 1=Reactor) | `0` (Proactor) |

Example:

```bash
./ModularNetServerApp -p 8080 -t 8 -m 1 -a 1
```

---

## Configuration

`src/config/server_config.json` provides hot-reloadable runtime configuration:

```json
{
    "log_level": 0,
    "log_close": true,
    "rate_limiter_close": false,
    "interceptor_close": false,
    "config_manager_close": true
}
```

- `log_level`: log level (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR, 4=FATAL)
- `log_close`: whether to disable log output
- `rate_limiter_close`: whether to disable the rate limiter
- `interceptor_close`: whether to disable the interceptor
- `config_manager_close`: whether to disable hot-reload

---

## Sample Endpoints

| Method | Path              | Description         |
| ------ | ----------------- | ------------------- |
| GET    | `/` `/index.html` | home page           |
| GET    | `/login.html`     | login page          |
| GET    | `/api/hello`      | JSON Hello          |
| GET    | `/api/time`       | current time        |
| GET    | `/api/info`       | server runtime info |
| GET    | `/checktoken`     | verify token        |
| POST   | `/api/echo`       | echo request body   |
| POST   | `/login`          | user login          |
| POST   | `/register`       | user registration   |
| POST   | `/api/tx_test`    | DB transaction test |
| WS     | `/ws_test.html`   | WebSocket test page |

---

## Design Highlights

1. **Pluggable multi-protocol**: `ConnFactoryManager` dispatches connection types by port; adding a new protocol only requires implementing `BaseConn` and registering a factory.
2. **Decoupled concurrency & trigger modes**: Reactor/Proactor and LT/ET can be freely combined via static members + config injection.
3. **Clean MVC layering**: Controller handles routing & parameters, Service handles business logic, DAO handles data access — single responsibility each.
4. **Built-in security**: interceptor + rate limiting + prepared statements + HMAC-SHA256 token auth protect every layer from ingress to data.
5. **Safe resource lifecycle**: connections managed by `shared_ptr`, DB connections/transactions by RAII, Session/Token auto-recycled by timers.
6. **Config hot-reload**: JSON config changes take effect without restart, suitable for production tuning.
