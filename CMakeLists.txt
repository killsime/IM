# 最低 CMake 版本要求
cmake_minimum_required(VERSION 3.10)

# 项目名称和版本
project(IMServer VERSION 1.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 添加库
add_library(MyServerLib
    main.cpp
    net/Connection.hpp
    net/Epoll.hpp
    net/EventLoop.hpp
    net/Socket.hpp
    net/Transfer/FileTransfer.hpp
    net/Transfer/TextTransfer.hpp
    net/Pack.hpp
    server/Message.hpp
    server/Msghander.hpp
    sql/Database.hpp
    sql/SqlCrud.hpp
    utils/Config.hpp
    utils/MQ.hpp
    utils/ThreadPool.hpp
)

# 添加头文件路径
target_include_directories(MyServerLib PRIVATE
    ${CMAKE_SOURCE_DIR}/net
    ${CMAKE_SOURCE_DIR}/server
    ${CMAKE_SOURCE_DIR}/sql
    ${CMAKE_SOURCE_DIR}/utils
)

# 查找并链接 pthread 库
find_package(Threads REQUIRED)
target_link_libraries(MyServerLib PRIVATE Threads::Threads)

# 手动设置 MySQL 的头文件和库文件路径
include_directories(/usr/include/mysql)
link_directories(/usr/lib/x86_64-linux-gnu)

# 链接 MySQL 库
target_link_libraries(MyServerLib PRIVATE mysqlclient)

# 添加可执行文件
add_executable(IMServer main.cpp)

# 链接库
target_link_libraries(IMServer PRIVATE MyServerLib)

# 如果需要测试，可以添加测试目录
enable_testing()
add_subdirectory(tests)