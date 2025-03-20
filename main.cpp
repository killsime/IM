#include "EventLoop.hpp"
#include "server/MsgHandler.hpp"
#include <iostream>

int main()
{
    // 初始化EventLoop
    EventLoop eventLoop;
    eventLoop.init();

    // 启动消息处理器
    MsgHandler msgHandler;
    msgHandler.start();

    // 运行EventLoop
    eventLoop.run();
    return 0;
}
