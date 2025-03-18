#ifndef CONNECTIONMGR_HPP
#define CONNECTIONMGR_HPP

#include <string>
#include <unordered_map>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>

// 网络信息结构体
struct NetInfo
{
    int client_fd;  // 客户端文件描述符
    bool online;    // 是否在线
    std::string ip; // 客户端 IP 地址

    NetInfo(int fd, const std::string &client_ip)
        : client_fd(fd), online(true), ip(client_ip) {}
};

// 连接管理类
class Connections
{
private:
    std::unordered_map<std::string, NetInfo> uidToNetInfo; // UID -> NetInfo
    std::mutex mtx;                                        // 保证线程安全

    // 根据 fd 获取 IP 地址
    std::string getIpByFd(int fd)
    {
        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(addr);
        if (getpeername(fd, (struct sockaddr *)&addr, &addr_len) == -1)
        {
            return "unknown";
        }
        return inet_ntoa(addr.sin_addr);
    }

public:
    // 添加连接
    void add(const std::string &uid, int fd)
    {
        std::lock_guard<std::mutex> lock(mtx);
        std::string ip = getIpByFd(fd);
        uidToNetInfo.emplace(uid, NetInfo(fd, ip));
        std::cout << "New connection: UID=" << uid << ", IP=" << ip << ", FD=" << fd << std::endl;
    }

    // 获取 fd
    int getFd(const std::string &uid)
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = uidToNetInfo.find(uid);
        if (it != uidToNetInfo.end())
        {
            return it->second.client_fd;
        }
        return -1; // 未找到
    }

    // 设置在线状态
    void setOnline(const std::string &uid, bool isOnline)
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = uidToNetInfo.find(uid);
        if (it != uidToNetInfo.end())
        {
            it->second.online = isOnline;
        }
    }

    // 扫描并关闭所有不在线的 fd
    void scanAndCloseInactive()
    {
        std::lock_guard<std::mutex> lock(mtx);
        for (auto it = uidToNetInfo.begin(); it != uidToNetInfo.end();)
        {
            if (!it->second.online)
            {
                close(it->second.client_fd); // 关闭 fd
                std::cout << "Closed inactive connection: UID=" << it->first << ", IP=" << it->second.ip << std::endl;
                it = uidToNetInfo.erase(it); // 从映射中移除
            }
            else
            {
                ++it;
            }
        }
    }
};

// 连接管理器
class ConnectionMgr
{
private:
    Connections connections; // 连接管理实例
    std::thread scanThread;  // 扫描线程
    bool running;            // 是否运行中

    // 单例模式
    ConnectionMgr() : running(true)
    {
        scanThread = std::thread(&ConnectionMgr::scanLoop, this);
    }
    ConnectionMgr(const ConnectionMgr &) = delete;
    ConnectionMgr &operator=(const ConnectionMgr &) = delete;

    // 扫描循环
    void scanLoop()
    {
        while (running)
        {
            std::this_thread::sleep_for(std::chrono::seconds(10)); // 每 10 秒扫描一次
            connections.scanAndCloseInactive();
        }
    }

public:
    ~ConnectionMgr()
    {
        running = false;
        if (scanThread.joinable())
        {
            scanThread.join();
        }
    }

    // 获取单例实例
    static ConnectionMgr &getInstance()
    {
        static ConnectionMgr instance;
        return instance;
    }

    // 获取连接管理实例
    Connections &getConnections()
    {
        return connections;
    }
};

#endif // CONNECTIONMGR_HPP