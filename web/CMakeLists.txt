# web 模块（HTTP接口）
add_library(web_lib INTERFACE)

# 头文件路径（其他模块#include "web/httplib.h"）
target_include_directories(web_lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

