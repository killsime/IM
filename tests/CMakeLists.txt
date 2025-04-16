# tests/CMakeLists.txt
# 查找依赖
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)


# 测试可执行文件
add_executable(MyServerTests
    integration/test_integration.cpp
    units/test_api.cpp
    # main/mysql.cpp
)

# 在测试目标的配置中添加
target_include_directories(MyServerTests PRIVATE
    ${CMAKE_SOURCE_DIR}/web  # 添加web目录到头文件搜索路径
    ${CMAKE_SOURCE_DIR}      # 添加项目根目录
)

# 设置C++标准
set_target_properties(MyServerTests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# 链接实际需要的模块
target_link_libraries(MyServerTests PRIVATE
    server_lib
    net_lib
    utils_lib
    GTest::GTest
    GTest::Main
    Threads::Threads
)

# 添加测试用例
gtest_discover_tests(MyServerTests)
