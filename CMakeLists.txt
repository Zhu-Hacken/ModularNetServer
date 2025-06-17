# 导出 compile_commands.json（供 clangd 使用）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(ModularNetServer)

cmake_minimum_required(VERSION 3.10)


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


include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/third_party
)

# 递归包含所有 src 子目录
file(GLOB_RECURSE HEADER_DIRS LIST_DIRECTORIES true ${PROJECT_SOURCE_DIR}/src/*)
foreach(header_dir ${HEADER_DIRS})
    if(IS_DIRECTORY ${header_dir})
        include_directories(${header_dir})
    endif()
endforeach()

# 源码文件（排除 main.cpp 和测试代码）
file(GLOB_RECURSE SOURCES "src/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX "src/test/.*")
list(FILTER SOURCES EXCLUDE REGEX "src/main.cpp")

# 构建为静态库
add_library(ModularNetServer STATIC ${SOURCES})
target_link_libraries(ModularNetServer stdc++fs mysqlclient ssl crypto hiredis)
message(STATUS "Sources: ${SOURCES}")

# add_executable(ModularNetServer src/main.cpp ${SOURCES})
# message(STATUS "optional: -fstandalone-debug")

# add_executable(test_cache src/test/test_cache.cpp ${SOURCES})
# target_link_libraries(test_cache stdc++fs mysqlclient ssl crypto hiredis pthread )