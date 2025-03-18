#include "net/Socket.hpp"
#include "net/ConnectionMgr.hpp"
#include <iostream>

int main()
{
    Socket server;
    server.initServer(9527);

    ConnectionMgr &mgr = ConnectionMgr::getInstance();

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
            mgr.getConnections().add(uid, client.getFd());
        }

        // 发送欢迎消息
        client.send("Welcome, Client!");
    }

    return 0;
}