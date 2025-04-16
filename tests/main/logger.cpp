
#include "net/EventLoop.hpp"
#include "server/MsgHandler.hpp"
#include "Logger.hpp"
#include <iostream>

int main()
{
    LOG_INFO("系统启动中...");

    // 初始化EventLoop
    EventLoop eventLoop;
    eventLoop.init();
    LOG_WARN("内存使用量达到 %d%%，接近警戒线", 85);
    LOG_ERROR("数据库连接失败: %s", "Connection refused");
    // 启动消息处理器.负责进行处理消息
    MsgHandler msgHandler;
    msgHandler.start();

    // 运行EventLoop,负责收发消息到消息队列
    eventLoop.run();
    LOG_INFO("系统关闭");

    return 0;
}
