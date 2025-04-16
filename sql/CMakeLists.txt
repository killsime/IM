# sql 模块（数据库）
add_library(sql_lib INTERFACE)

# 头文件路径
target_include_directories(sql_lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 依赖 utils_lib
target_link_libraries(sql_lib INTERFACE
    utils_lib
)

# 可选：MySQL配置（如需取消注释）
# find_package(MySQL REQUIRED)
# target_link_libraries(sql_lib INTERFACE MySQL::MySQL)

