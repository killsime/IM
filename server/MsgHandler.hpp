#ifndef MSGHANDLER_HPP
#define MSGHANDLER_HPP

#include "MQ.hpp"
#include "Message.hpp"
#include "../net/ConnectionMgr.hpp"
#include "../net/FileTransfer.hpp"
#include "../utils/ThreadPool.hpp"
#include <iostream>
#include <sstream>
#include <thread>
#include <memory>

class MsgHandler
{
public:
    MsgHandler() : mq(MessageQueue::getInstance()) {}

    void start()
    {
        std::thread([this]()
                    { processMessages(); })
            .detach();
    }

private:
    void processMessages()
    {
        while (true)
        {
            Message msg = mq.popFromRecvQueue();
            switch (msg.type)
            {
            case Message::Type::TEXT:
                handleText(msg);
                break;
            case Message::Type::FILE:
                handleFile(msg);
                break;
            default:
                break;
            }
        }
    }

    void handleText(const Message &msg)
    {
        auto &text = *static_cast<const TextData *>(msg.data.get());
        auto &connMgr = ConnectionMgr::getInstance().getTextConnections();
        auto connections = connMgr.getConnections();

        // 广播给所有在线用户（排除发送者）
        for (const auto &conn : connections)
        {
            if (conn.first != text.sender && conn.second.online)
            {
                TextData broadcast = text;
                broadcast.receiver = conn.first; // 直接使用 uint32_t 类型的 uid
                mq.pushToSendQueue(Message(broadcast));
            }
        }
    }

    void handleFile(const Message &msg)
    {
        auto &file = *static_cast<const FileData *>(msg.data.get());
        auto &connMgr = ConnectionMgr::getInstance().getIOConnections();
        Socket clientSocket = connMgr.getSocket(file.sender); // 直接使用 uint32_t 类型的 uid

        if (file.action == FileAction::UPLOAD)
        {
            ThreadPool::getInstance().enqueue([this, &clientSocket, file]()
                                              { handleFileUpload(clientSocket, file); });
        }
        else if (file.action == FileAction::DOWNLOAD)
        {
            ThreadPool::getInstance().enqueue([this, &clientSocket, file]()
                                              { handleFileDownload(clientSocket, file); });
        }
    }

    void handleFileUpload(Socket &clientSocket, const FileData &file)
    {
        std::string fileName = file.filename.data();
        FileTransfer transfer(clientSocket);

        if (!transfer.receiveFile(fileName))
        {
            std::cerr << "Failed to receive file: " << fileName << std::endl;
            ConnectionMgr::getInstance().getIOConnections().removeConnection(file.sender);

            return;
        }

        std::cout << "File received and saved to: " << fileName << std::endl;
        ConnectionMgr::getInstance().getIOConnections().removeConnection(file.sender);
        sendFileNotification(file);
    }

    void handleFileDownload(Socket &clientSocket, const FileData &file)
    {
        std::string fileName = file.filename.data();
        FileTransfer transfer(clientSocket);

        if (!transfer.sendFile(fileName))
        {
            std::cerr << "Failed to send file: " << fileName << std::endl;
            ConnectionMgr::getInstance().getIOConnections().removeConnection(file.receiver);
            return;
        }

        ConnectionMgr::getInstance().getIOConnections().removeConnection(file.receiver);
        std::cout << "File sent successfully: " << fileName << std::endl;
    }

    void sendFileNotification(const FileData &file)
    {
        std::stringstream ss;
        ss << "[文件] " << file.filename.data()
           << " (" << file.filesize << "字节)";

        TextData notification{
            file.sender,
            file.receiver,
            {},
            TextType::GROUP};
        ss.read(&notification.content[0], notification.content.size() - 1);

        mq.pushToSendQueue(Message(notification));
    }

    MessageQueue &mq;
};

#endif // MSGHANDLER_HPP