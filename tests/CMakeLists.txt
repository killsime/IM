# 添加测试可执行文件
add_executable(MyServerTests
    integration/test_integration.cpp
    units/test_units.cpp
)

# 添加头文件路径
target_include_directories(MyServerTests PRIVATE
    ${CMAKE_SOURCE_DIR}/net
    ${CMAKE_SOURCE_DIR}/server
    ${CMAKE_SOURCE_DIR}/sql
    ${CMAKE_SOURCE_DIR}/utils
)

# 链接主项目的库
target_link_libraries(MyServerTests PRIVATE MyServerLib)

# 链接 pthread 库
target_link_libraries(MyServerTests PRIVATE Threads::Threads)

# 链接 MySQL 库
target_link_libraries(MyServerTests PRIVATE mysqlclient)

# 添加测试
add_test(NAME MyServerTests COMMAND MyServerTests)