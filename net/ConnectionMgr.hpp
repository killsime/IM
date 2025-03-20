#ifndef CONNECTIONMGR_HPP
#define CONNECTIONMGR_HPP

#include "Socket.hpp" // 引用 Socket 类
#include <string>
#include <unordered_map>
#include <iostream>
#include <mutex>
#include <functional>
#include <chrono>
#include <memory>
#include <thread>
#include <array>

// 网络信息结构体
struct NetInfo
{
    Socket socket;  // 客户端 Socket
    bool online;    // 是否在线
    std::string ip; // 客户端 IP 地址

    NetInfo(const Socket &sock, const std::string &client_ip)
        : socket(sock), online(true), ip(client_ip) {}
};

// 连接管理基类
class Connections
{
protected:
    std::unordered_map<uint32_t, NetInfo> uidToNetInfo; // UID -> NetInfo
    std::mutex mtx;                                     // 保证线程安全

    // 根据 Socket 获取 IP 地址
    std::string getIpBySocket(const Socket &socket)
    {
        return socket.getRemoteIp(); // 假设 Socket 类提供了获取远程 IP 的方法
    }

public:
    // 添加连接
    void add(uint32_t uid, const Socket &socket)
    {
        std::lock_guard<std::mutex> lock(mtx);
        std::string ip = getIpBySocket(socket);
        uidToNetInfo.emplace(uid, NetInfo(socket, ip));
        std::cout << "New connection: UID=" << uid << ", IP=" << ip << ", FD=" << socket.getFd() << std::endl;
    }

    // 获取 Socket
    Socket getSocket(uint32_t uid)
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = uidToNetInfo.find(uid);
        if (it != uidToNetInfo.end())
        {
            return it->second.socket;
        }
        return Socket(); // 返回一个无效的 Socket
    }

    // 设置在线状态
    void setOnline(uint32_t uid, bool isOnline)
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = uidToNetInfo.find(uid);
        if (it != uidToNetInfo.end())
        {
            it->second.online = isOnline;
        }
    }

    // 移除某个在线状态
    void removeConnection(uint32_t uid)
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = uidToNetInfo.find(uid);
        if (it != uidToNetInfo.end())
        {
            it->second.socket.close(); // 关闭 Socket
            std::cout << "Removed connection: UID=" << uid << ", IP=" << it->second.ip << std::endl;
            uidToNetInfo.erase(it); // 从映射中移除
        }
    }

    // 获取所有连接
    std::unordered_map<uint32_t, NetInfo> getConnections()
    {
        std::lock_guard<std::mutex> lock(mtx);
        return uidToNetInfo;
    }

    // 扫描并关闭所有不在线的 Socket
    void scanAndCloseInactive()
    {
        std::lock_guard<std::mutex> lock(mtx);
        for (auto it = uidToNetInfo.begin(); it != uidToNetInfo.end();)
        {
            if (!it->second.online)
            {
                it->second.socket.close(); // 关闭 Socket
                std::cout << "Closed inactive connection: UID=" << it->first << ", IP=" << it->second.ip << std::endl;
                it = uidToNetInfo.erase(it); // 从映射中移除
            }
            else
            {
                it->second.online = false;
                ++it;
            }
        }
    }
};

// 文本消息连接管理
class TextConnection : public Connections
{
    // 可以扩展文本消息特有的功能
};

// IO 连接管理
class IOConnection : public Connections
{
    // 可以扩展 IO 特有的功能
};

// 连接管理器
class ConnectionMgr
{
private:
    TextConnection textConnections; // 文本消息连接管理
    IOConnection ioConnections;     // IO 连接管理

    // 单例模式
    ConnectionMgr() = default;
    ConnectionMgr(const ConnectionMgr &) = delete;
    ConnectionMgr &operator=(const ConnectionMgr &) = delete;

public:
    // 获取单例实例
    static ConnectionMgr &getInstance()
    {
        static ConnectionMgr instance;
        return instance;
    }

    // 获取文本消息连接管理实例
    TextConnection &getTextConnections()
    {
        return textConnections;
    }

    // 获取 IO 连接管理实例
    IOConnection &getIOConnections()
    {
        return ioConnections;
    }

    // 启动定时器扫描
    void startScanTimer(std::function<void()> callback, int intervalSeconds)
    {
        std::thread([callback, intervalSeconds]()
                    {
            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
                callback();
            } })
            .detach();
    }
};

#endif // CONNECTIONMGR_HPP