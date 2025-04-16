# utils 模块（基础工具库）
add_library(utils_lib INTERFACE)

# 暴露当前目录为头文件路径（其他模块#include "utils/Logger.hpp"）
target_include_directories(utils_lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 设置C++11标准（传递给依赖此库的目标）
target_compile_features(utils_lib INTERFACE cxx_std_11)

# 链接系统线程库（传递依赖）
find_package(Threads REQUIRED)
target_link_libraries(utils_lib INTERFACE
    Threads::Threads
)

