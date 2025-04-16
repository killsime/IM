cmake_minimum_required(VERSION 3.10)
project(IMServer VERSION 1.0 LANGUAGES CXX)

# 全局设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 查找线程库（所有模块共享）
find_package(Threads REQUIRED)

# 添加模块（按依赖顺序）
add_subdirectory(utils)
add_subdirectory(net)
add_subdirectory(sql)
add_subdirectory(web)
add_subdirectory(server)

# 主程序
add_executable(IMServer main.cpp)
target_link_libraries(IMServer PRIVATE
    server_lib  # 自动传递 net_lib 和 utils_lib
    web_lib
    Threads::Threads
)

# 测试目录
enable_testing()
add_subdirectory(tests)