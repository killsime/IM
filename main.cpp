#include "net/EventLoop.hpp"
#include "server/MsgHandler.hpp"
#include <iostream>

int main()
{
    // 初始化EventLoop
    EventLoop eventLoop;
    eventLoop.init();

    // 启动消息处理器.负责进行处理消息
    MsgHandler msgHandler;
    msgHandler.start();

    // 运行EventLoop,负责收发消息到消息队列
    eventLoop.run();
    return 0;
}
