#ifndef EPOLL_HPP
#define EPOLL_HPP

#include <sys/epoll.h>
#include <unistd.h>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <cstring>    // for strerror
#include "Socket.hpp" // 包含 Socket 类的头文件

// 定义常量宏
#define MAX_EVENTS 1000      // 每次等待的最大事件数
#define EPOLL_CREATE_FLAGS 0 // epoll_create1 的标志
#define EPOLL_ERROR -1       // epoll 错误返回值
#define DEFAULT_TIMEOUT -1   // 默认超时时间（阻塞等待）

class Epoll
{
public:
    Epoll()
    {
        epoll_fd = epoll_create1(EPOLL_CREATE_FLAGS);
        if (epoll_fd == EPOLL_ERROR)
        {
            printf("Failed to create epoll instance: %s\n", strerror(errno));
            return;
        }
    }

    ~Epoll()
    {
        if (epoll_fd != EPOLL_ERROR)
        {
            close(epoll_fd);
            printf("Epoll instance closed.\n");
        }
    }

    // 添加 Socket 到 epoll
    bool add(const Socket &socket, uint32_t events = EPOLLIN | EPOLLRDHUP)
    {
        int fd = socket.getFd();
        if (fd == INVALID_SOCKET)
        {
            printf("Invalid socket fd.\n");
            return false;
        }

        struct epoll_event event;
        event.events = events;
        event.data.fd = fd;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) == EPOLL_ERROR)
        {
            printf("Failed to add fd to epoll: %s\n", strerror(errno));
            return false;
        }
        printf("Added fd: %d to epoll.\n", fd);
        return true;
    }

    // 从 epoll 中删除 Socket
    void del(const Socket &socket)
    {
        int fd = socket.getFd();
        if (fd == INVALID_SOCKET)
        {
            printf("Invalid socket fd.\n");
            return;
        }

        if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr) == EPOLL_ERROR)
        {
            printf("Failed to delete fd from epoll: %s\n", strerror(errno));
            return;
        }
        printf("Deleted fd: %d from epoll.\n", fd);
    }

    // 等待事件发生，返回触发事件的 Socket 列表
    std::vector<Socket> wait(int timeout = DEFAULT_TIMEOUT)
    {
        struct epoll_event events[MAX_EVENTS];
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, timeout);
        if (num_events == EPOLL_ERROR)
        {
            printf("Failed to wait for epoll events: %s\n", strerror(errno));
            return {};
        }

        std::vector<Socket> active_sockets;
        for (int i = 0; i < num_events; ++i)
        {
            active_sockets.push_back(Socket(events[i].data.fd));
        }
        return active_sockets;
    }

private:
    int epoll_fd; // epoll 文件描述符
};

#endif // EPOLL_HPP