#pragma once

#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <string>
#include <stdexcept>
#include <iostream>
#include <cstdio> // for printf

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define SOCKET_ERROR (-1)
    #define INVALID_SOCKET (~0)
    #define GET_LAST_ERROR WSAGetLastError()
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <fcntl.h>
    #define SOCKET_ERROR (-1)
    #define INVALID_SOCKET (-1)
    #define GET_LAST_ERROR errno
#endif

// 定义常量宏
#define BUFFER_SIZE 1024   // 接收缓冲区大小
#define DEFAULT_PORT 9527  // 默认端口号
#define DEFAULT_IP "127.0.0.1" // 默认 IP 地址
#define MAX_BACKLOG 1000   // 最大连接队列长度

class Socket {
public:
    // 默认构造函数
    Socket() : fd(INVALID_SOCKET) {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            printf("WSAStartup failed.\n");
            return;
        }
#endif
    }

    // 服务端处理 Socket 的构造函数
    Socket(int client_fd) : fd(client_fd) {}

    // 析构函数
    ~Socket() {
#ifdef _WIN32
        WSACleanup();
#endif
    }

    // 关闭 Socket
    void close() {
        if (fd != INVALID_SOCKET) {
#ifdef _WIN32
            closesocket(fd);
#else
            ::close(fd);
#endif
            fd = INVALID_SOCKET;
            printf("Socket closed.\n");
        }
    }

    // 初始化客户端
    void initClient(const std::string& ip = DEFAULT_IP, int port = DEFAULT_PORT) {
        fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (fd == INVALID_SOCKET) {
            printf("Failed to create socket.\n");
            return;
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

        if (::connect(fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            close();
            printf("Failed to connect to server.\n");
            return;
        }
    }

    // 初始化服务端
    void initServer(int port = DEFAULT_PORT) {
        fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (fd == INVALID_SOCKET) {
            printf("Failed to create socket.\n");
            return;
        }

        // 设置 SO_REUSEADDR 选项
        int opt = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
            printf("Failed to set SO_REUSEADDR.\n");
            return;
        }

        // 绑定地址和端口
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        if (bind(fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            printf("Failed to bind socket.\n");
            return;
        }

        // 开始监听
        if (listen(fd, MAX_BACKLOG) == SOCKET_ERROR) {
            printf("Failed to listen on socket.\n");
            return;
        }
    }

    // 服务端接受客户端连接
    Socket accept() {
        if (fd == INVALID_SOCKET) {
            printf("Not a server socket or not initialized.\n");
            return Socket(INVALID_SOCKET);
        }

        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int client_fd = ::accept(fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_fd == INVALID_SOCKET) {
            printf("Failed to accept client connection.\n");
            return Socket(INVALID_SOCKET);
        }

        return Socket(client_fd);
    }

    // 发送数据
    bool send(const std::string& data) {
        if (fd == INVALID_SOCKET) {
            printf("Invalid socket.\n");
            return false;
        }

        int bytes_sent = ::send(fd, data.c_str(), static_cast<int>(data.size()), 0);
        if (bytes_sent == SOCKET_ERROR) {
            printf("Failed to send data.\n");
            return false;
        }
        return true;
    }

    // 接收数据
    bool recv(std::string& buffer) {
        if (fd == INVALID_SOCKET) {
            printf("Invalid socket.\n");
            return false;
        }

        char temp_buffer[BUFFER_SIZE];
        int bytes_received = ::recv(fd, temp_buffer, sizeof(temp_buffer), 0);
        if (bytes_received == SOCKET_ERROR) {
            printf("Failed to receive data.\n");
            return false;
        } else if (bytes_received == 0) {
            // 对端关闭连接
            printf("Connection closed by peer.\n");
            return false;
        }

        buffer.assign(temp_buffer, bytes_received);
        return true;
    }

    // 获取文件描述符
    int getFd() const { return fd; }

private:
    int fd; // 文件描述符
};

#endif // SOCKET_HPP