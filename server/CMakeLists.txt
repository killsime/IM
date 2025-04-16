# server 模块（业务逻辑）
add_library(server_lib INTERFACE)

# 头文件路径
target_include_directories(server_lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 依赖 net_lib 和 utils_lib
target_link_libraries(server_lib INTERFACE
    net_lib
    utils_lib
)

