#include "net/Epoll.hpp"
#include "net/Socket.hpp"
#include <iostream>

int main() {
    // 创建服务端 Socket
    Socket server;
    server.initServer(9527); // 初始化服务端

    // 创建 Epoll 实例
    Epoll epoll;
    epoll.add(server); // 将服务端 Socket 添加到 epoll

    while (true) {
        // 等待事件
        std::vector<Socket> active_sockets = epoll.wait();
        for (Socket& socket : active_sockets) {
            if (socket.getFd() == server.getFd()) {
                // 有新客户端连接
                Socket client = server.accept();
                if (client.getFd() != INVALID_SOCKET) {
                    printf("New client connected: fd=%d\n", client.getFd());
                    epoll.add(client); // 将客户端 Socket 添加到 epoll
                }
            } else {
                // 客户端 Socket 有数据可读
                std::string data;
                if (socket.recv(data)) {
                    printf("Received data from fd=%d: %s\n", socket.getFd(), data.c_str());
                    socket.send("Hello from server!"); // 发送响应
                } else {
                    // 客户端断开连接
                    printf("Client fd=%d disconnected.\n", socket.getFd());
                    epoll.del(socket); // 从 epoll 中删除客户端 Socket
                    socket.close();
                }
            }
        }
    }

    return 0;
}