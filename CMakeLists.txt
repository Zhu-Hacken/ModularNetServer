cmake_minimum_required(VERSION 3.10)
project(ModularNetServer)

# 导出 compile_commands.json（供 clangd 使用）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 使用 C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 默认 Debug
if(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE Debug)
endif()

# Debug 编译参数（兼容 valgrind）
set(CMAKE_CXX_FLAGS_DEBUG "-g -gdwarf-4 -O0 -Wall -Wextra")
add_compile_options(-Wunused-parameter -Wno-unused-parameter -Wno-reorder -Wreorder)

# === 头文件与源码收集 ===
# 源码文件（排除 main.cpp 和测试代码）
file(GLOB_RECURSE SOURCES "src/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX "src/test/.*")
list(FILTER SOURCES EXCLUDE REGEX "src/main.cpp")

include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/third_party
)

# 递归包含所有 src 子目录
file(GLOB_RECURSE HEADER_DIRS LIST_DIRECTORIES true ${PROJECT_SOURCE_DIR}/src/*)
set(INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src)
foreach(dir ${HEADER_DIRS})
    if(IS_DIRECTORY ${dir})
        list(APPEND INCLUDE_DIRS ${dir})
    endif()
endforeach()

# 构建为静态库
add_library(ModularNetServer STATIC ${SOURCES})

# 设置 include 路径（PUBLIC 表示向外暴露）
target_include_directories(ModularNetServer PUBLIC ${INCLUDE_DIRS})

# 链接系统库
target_link_libraries(ModularNetServer stdc++fs mysqlclient ssl crypto hiredis)

# 显示源文件列表
message(STATUS "Sources: ${SOURCES}")

# add_executable(ModularNetServer src/main.cpp ${SOURCES})
# message(STATUS "optional: -fstandalone-debug")

# add_executable(test_cache src/test/test_cache.cpp ${SOURCES})
# target_link_libraries(test_cache stdc++fs mysqlclient ssl crypto hiredis pthread )