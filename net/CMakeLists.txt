# net 模块（网络核心）
add_library(net_lib INTERFACE)

# 头文件路径（其他模块#include "net/Epoll.hpp"）
target_include_directories(net_lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 依赖 utils_lib
target_link_libraries(net_lib INTERFACE
    utils_lib
)
