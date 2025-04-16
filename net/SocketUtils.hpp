#ifndef SOCKET_UTILS_HPP
#define SOCKET_UTILS_HPP

#include "Socket.hpp"
#include <string>

class SocketUtils
{
public:
    // 获取远程IP地址
    static std::string getRemoteIp(const Socket& socket)
    {
        int fd = socket.getFd();
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

    // 优化大文件传输
    static void optimizeForLargeFileTransfer(Socket& socket)
    {
        int fd = socket.getFd();
        if (fd == INVALID_SOCKET)
        {
            return;
        }

        // 设置内核接收缓冲区大小为 1MB
        int kernel_buffer_size = 1024 * 1024; // 1MB
        if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char *)&kernel_buffer_size, sizeof(kernel_buffer_size)) == SOCKET_ERROR)
        {
            printf("Failed to set kernel receive buffer size to 1MB. Error: %d\n", GET_LAST_ERROR);
        }
    }
};

#endif // SOCKET_UTILS_HPP