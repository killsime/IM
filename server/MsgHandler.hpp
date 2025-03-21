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
    MsgHandler()
        : mq(MessageQueue::getInstance()),
          ioConn(ConnectionMgr::getInstance().getIOConnections()),
          txtConn(ConnectionMgr::getInstance().getTextConnections()) {}

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
        auto connections = txtConn.getConnections();

        // 广播给所有在线用户（排除发送者）
        for (const auto &conn : connections)
        {
            if (conn.first != text.sender && conn.second.online)
            {
                TextData broadcast = text;
                broadcast.receiver = conn.first;
                mq.pushToSendQueue(Message(broadcast));
            }
        }
    }

    void handleFile(const Message &msg)
    {
        auto &file = *static_cast<const FileData *>(msg.data.get());
        Socket clientSocket = ioConn.getSocket(file.sender);

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
            ioConn.removeConnection(file.sender);
            return;
        }

        ioConn.removeConnection(file.sender);
        sendFileNotification(file);
    }

    void handleFileDownload(Socket &clientSocket, const FileData &file)
    {
        std::string fileName = file.filename.data();
        FileTransfer transfer(clientSocket);

        if (!transfer.sendFile(fileName))
        {
            std::cerr << "Failed to send file: " << fileName << std::endl;
            ioConn.removeConnection(file.sender);
            return;
        }

        ioConn.removeConnection(file.sender);
    }

    void sendFileNotification(const FileData &file)
    {
        // 在循环外部构造文件信息消息内容
        std::string content = "[filename] " + std::string(file.filename.data()) +
                              " (" + std::to_string(file.filesize) + "byte)";

        // 构造一个基础通知消息
        TextData notification{
            file.sender,      // 发送者UID
            0,                // 接收者UID（稍后在循环中替换）
            {},               // 消息内容
            TextType::GROUP}; // 消息类型：群发

        // 将文件信息写入消息内容
        std::copy(content.begin(), content.end(), notification.content.begin());
        notification.content[content.size()] = '\0'; // 确保消息内容以 null 结尾

        // 获取所有在线连接
        auto connections = txtConn.getConnections();

        // 广播给所有在线用户（排除发送者）
        for (const auto &conn : connections)
        {
            if (conn.first != file.sender && conn.second.online)
            {
                // 只替换接收者
                notification.receiver = conn.first;

                // 将消息放入发送队列
                mq.pushToSendQueue(Message(notification));
            }
        }
    }

    MessageQueue &mq;
    IOConnection &ioConn;
    TextConnection &txtConn;
};

#endif // MSGHANDLER_HPP