#include "EventLoop.hpp"
#include "server/MsgHandler.hpp"
#include <iostream>

int main()
{
    // 初始化EventLoop
    EventLoop eventLoop;
    if (!eventLoop.init())
    {
        std::cerr << "Failed to initialize EventLoop" << std::endl;
        return 1;
    }

    // 启动消息处理器
    MsgHandler msgHandler;
    msgHandler.start();

    // 运行EventLoop
    eventLoop.run();
    return 0;
}