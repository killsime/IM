#include "net/Epoll.hpp"
#include "net/Socket.hpp"
#include "net/ConnectionMgr.hpp"
#include <iostream>

int main()
{
    Socket server;
    server.initServer(9527);

    ConnectionMgr &mgr = ConnectionMgr::getInstance();

    // 启动定时器，每 10 秒扫描一次
    mgr.startScanTimer([]()
                       {
        std::cout << "Scanning inactive connections..." << std::endl;
        ConnectionMgr::getInstance().getTextConnections().scanAndCloseInactive();
        ConnectionMgr::getInstance().getIOConnections().scanAndCloseInactive(); }, 10);

    while (1)
    {
        Socket client = server.accept();
        if (client.getFd() == -1)
        {
            continue;
        }

        // 接收第一个消息作为 UID
        std::string uid;
        if (client.recv(uid))
        {
            std::cout << "Received UID: " << uid << std::endl;
            // 将连接添加到文本消息管理
            mgr.getTextConnections().add(uid, client.getFd());
        }

        // 发送欢迎消息
        client.send("Welcome, Client!");
    }

    return 0;
}