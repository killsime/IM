#ifndef EVENTLOOP_HPP
#define EVENTLOOP_HPP

#include "net/Epoll.hpp"
#include "net/Socket.hpp"
#include "server/MQ.hpp"
#include "server/Message.hpp"
#include "net/ConnectionMgr.hpp"
#include "net/Pack.hpp"
#include <unordered_map>
#include <iostream>
#include <thread>
#include <chrono>

#define MSG_PORT 9527
#define FILE_PORT 9528

class EventLoop
{
public:
    EventLoop() {}

    bool init()
    {
        if (!msgSocket_.initServer(MSG_PORT) || !fileSocket_.initServer(FILE_PORT))
        {
            std::cerr << "Socket initialization failed" << std::endl;
            return false;
        }

        if (!epoll_.add(msgSocket_, EPOLLIN | EPOLLRDHUP) ||
            !epoll_.add(fileSocket_, EPOLLIN | EPOLLRDHUP))
        {
            std::cerr << "Epoll add failed" << std::endl;
            return false;
        }

        startHeartbeatChecker();
        std::thread([this]()
                    { sendMessages(); })
            .detach();
        return true;
    }

    void run()
    {
        while (true)
        {
            auto sockets = epoll_.wait();
            for (auto &sock : sockets)
            {
                int fd = sock.getFd();
                if (fd == msgSocket_.getFd())
                    handleNewConnection(msgSocket_);
                else if (fd == fileSocket_.getFd())
                    handleNewConnection(fileSocket_);
                else
                    handleClientData(fd);
            }
        }
    }

private:
    void handleNewConnection(Socket &listener)
    {
        Socket client = listener.accept();
        if (client.getFd() == INVALID_SOCKET)
            return;

        epoll_.add(client, EPOLLIN | EPOLLET);
    }

    void handleClientData(int fd)
    {
        std::vector<char> data;
        if (!Socket(fd).recv(data))
        {
            cleanupClient(fd);
            return;
        }

        try
        {
            Pack pack(data);
            Message msg = parsePack(pack);
            preprocessMessage(fd, msg);
            MessageQueue::getInstance().pushToRecvQueue(std::move(msg));
        }
        catch (const std::exception &e)
        {
            std::cerr << "Pack error: " << e.what() << std::endl;
        }
    }

    Message parsePack(const Pack &pack)
    {
        switch (pack.getType())
        {
        case 1:
            return Message(*reinterpret_cast<const UserData *>(pack.getData().data()));
        case 2:
            return Message(*reinterpret_cast<const TextData *>(pack.getData().data()));
        case 3:
            return Message(*reinterpret_cast<const FileData *>(pack.getData().data()));
        default:
            throw std::runtime_error("Unknown pack type");
        }
    }

    void preprocessMessage(int fd, const Message &msg)
    {
        if (msg.type == Message::Type::USER)
        {
            auto &user = *static_cast<UserData *>(msg.data.get());
            auto &conn = ConnectionMgr::getInstance().getTextConnections();

            switch (user.action)
            {
            case UserAction::LOGIN:
                conn.add(user.uid, Socket(fd));
                break;
            case UserAction::LOGOUT:
                conn.setOnline(user.uid, false);
                break;
            case UserAction::HEARTBEAT:
                conn.setOnline(user.uid, true);
                break;
            default:
                break;
            }
        }
        else if (msg.type == Message::Type::FILE)
        {
            auto &file = *static_cast<FileData *>(msg.data.get());
            auto &conn = ConnectionMgr::getInstance().getIOConnections();
            conn.add(file.sender, Socket(fd));
            epoll_.del(Socket(fd));
        }
    }

    void cleanupClient(int fd)
    {
        epoll_.del(Socket(fd));
        ConnectionMgr::getInstance().getTextConnections().setOnline(fd, false);
        std::cout << "Client disconnected: " << fd << std::endl;
    }

    void startHeartbeatChecker()
    {
        ConnectionMgr::getInstance().startScanTimer([this]()
                                                    { ConnectionMgr::getInstance().getTextConnections().scanAndCloseInactive(); }, 600);
    }

    void sendMessages()
    {
        MessageQueue &mq = MessageQueue::getInstance();
        while (true)
        {
            if (!mq.isSendQueueEmpty())
            {
                Message msg = mq.popFromSendQueue();
                if (msg.type == Message::Type::TEXT)
                {
                    auto &text = *static_cast<TextData *>(msg.data.get());
                    Socket clientSocket = ConnectionMgr::getInstance()
                                              .getTextConnections()
                                              .getSocket(text.receiver);

                    if (clientSocket.getFd())
                    {
                        std::vector<char> data(reinterpret_cast<char *>(&text), reinterpret_cast<char *>(&text) + sizeof(TextData));
                        Pack pack(2, data);
                        clientSocket.send(pack.toByteStream());
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    Socket msgSocket_;
    Socket fileSocket_;
    Epoll epoll_;
};

#endif // EVENTLOOP_HPP