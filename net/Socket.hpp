#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <vector>
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
#include <netinet/tcp.h>
#define SOCKET_ERROR (-1)
#define INVALID_SOCKET (-1)
#define GET_LAST_ERROR errno
#endif

// 定义常量宏
#define BUFFER_SIZE 1024       // 接收缓冲区大小
#define DEFAULT_PORT 9527      // 默认端口号
#define DEFAULT_IP "127.0.0.1" // 默认 IP 地址
#define MAX_BACKLOG 1000       // 最大连接队列长度

class Socket
{

private:
    int fd;             // 文件描述符
    size_t buffer_size; // 接收缓冲区大小

public:
    // 默认构造函数
    Socket() : fd(INVALID_SOCKET), buffer_size(BUFFER_SIZE)
    {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            printf("WSAStartup failed.\n");
            return;
        }
#endif
    }

    // 服务端处理 Socket 的构造函数
    Socket(int client_fd) : fd(client_fd), buffer_size(BUFFER_SIZE) {}

    // 析构函数
    ~Socket()
    {
#ifdef _WIN32
        WSACleanup();
#endif
    }

    // 关闭 Socket
    void close()
    {
        if (fd != INVALID_SOCKET)
        {
#ifdef _WIN32
            closesocket(fd);
#else
            ::close(fd);
#endif
            // printf("Socket closed.fd : %d\n",fd);
            fd = INVALID_SOCKET;
        }
    }

    // 初始化客户端
    bool initClient(const std::string &ip = DEFAULT_IP, int port = DEFAULT_PORT)
    {
        fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (fd == INVALID_SOCKET)
        {
            printf("Failed to create socket.\n");
            return false;
        }

        // 禁用 Nagle 算法
        int flag = 1;
        if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag)) == SOCKET_ERROR)
        {
            printf("Failed to disable Nagle (TCP_NODELAY).\n");
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

        if (::connect(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
        {
            close();
            printf("Failed to connect to server.\n");
            return false;
        }

        return true;
    }

    // 初始化服务端
    bool initServer(int port = DEFAULT_PORT)
    {
        fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (fd == INVALID_SOCKET)
        {
            printf("Failed to create socket.\n");
            return false;
        }

        // 设置 SO_REUSEADDR 选项
        int opt = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) == SOCKET_ERROR)
        {
            printf("Failed to set SO_REUSEADDR.\n");
            return false;
        }

        // 禁用 Nagle 算法
        int flag = 1;
        if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag)) == SOCKET_ERROR)
        {
            printf("Failed to disable Nagle (_NODELAY).\n");
        }

        // 绑定地址和端口
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        if (bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
        {
            printf("Failed to bind socket.\n");
            return false;
        }

        // 开始监听
        if (listen(fd, MAX_BACKLOG) == SOCKET_ERROR)
        {
            printf("Failed to listen on socket.\n");
            return false;
        }

        return true;
    }

    // 服务端接受客户端连接
    Socket accept()
    {
        if (fd == INVALID_SOCKET)
        {
            printf("Not a server socket or not initialized.\n");
            return Socket(INVALID_SOCKET);
        }

        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int client_fd = ::accept(fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd == INVALID_SOCKET)
        {
            printf("Failed to accept client connection.\n");
            return Socket(INVALID_SOCKET);
        }

        return Socket(client_fd);
    }

    // 发送数据，返回实际发送的字节数（0表示失败）
    size_t send(const std::vector<char> &data)
    {
        if (fd == INVALID_SOCKET)
        {
            // printf("Invalid socket.\n");
            return 0;
        }

        int bytes_sent = ::send(fd, data.data(), static_cast<int>(data.size()), 0);
        if (bytes_sent == SOCKET_ERROR)
        {
            printf("Failed to send data.\n");
            return 0;
        }
        return static_cast<size_t>(bytes_sent);
    }

    // 接收数据，返回实际接收的字节数（0表示失败）
    size_t recv(std::vector<char> &buffer)
    {
        if (fd == INVALID_SOCKET)
        {
            // printf("Invalid socket.\n");
            return 0;
        }

        // 使用 vector 作为动态分配的缓冲区
        std::vector<char> temp_buffer(buffer_size);

        int bytes_received = ::recv(fd, temp_buffer.data(), static_cast<int>(temp_buffer.size()), 0);
        if (bytes_received == SOCKET_ERROR)
        {
            printf("Failed to receive data.\n");
            return 0;
        }
        else if (bytes_received == 0)
        {
            // 对端关闭连接
            // printf("Connection closed by peer.\n");
            return 0;
        }

        // 将接收到的数据复制到输出缓冲区
        buffer.assign(temp_buffer.begin(), temp_buffer.begin() + bytes_received);
        return static_cast<size_t>(bytes_received);
    }

    // 在 Socket 类中添加以下方法
    std::string getRemoteIp() const
    {
        if (fd == INVALID_SOCKET)
        {
            return "unknown";
        }

        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(addr);
        if (getpeername(fd, (struct sockaddr *)&addr, &addr_len) == -1)
        {
            return "unknown";
        }
        return inet_ntoa(addr.sin_addr);
    }

    void optimizeForLargeFileTransfer()
    {
        if (fd == INVALID_SOCKET)
        {
            // printf("Invalid socket.\n");
            return;
        }

        // 设置内核接收缓冲区大小为 1MB
        int kernel_buffer_size = 1024 * 1024; // 1MB
        if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char *)&kernel_buffer_size, sizeof(kernel_buffer_size)) == SOCKET_ERROR)
        {
            printf("Failed to set kernel receive buffer size to 1MB. Error: %d\n", GET_LAST_ERROR);
        }

        // 设置应用层接收缓冲区大小为 64KB
        buffer_size = 64 * 1024; // 64KB
    }
    // 获取文件描述符
    int getFd() const { return fd; }
};

#endif // SOCKET_HPP