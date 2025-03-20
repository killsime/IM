#ifndef MSGHANDLER_HPP
#define MSGHANDLER_HPP

#include "MQ.hpp"
#include "Message.hpp"
#include "../net/ConnectionMgr.hpp"
#include "../net/FileTransfer.hpp"
#include "../utils/ThreadPool.hpp"
#include <iostream>
#include <sstream>

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
        for (const auto &[uid, netInfo] : connections)
        {
            if (netInfo.client_fd != text.sender && netInfo.online)
            {
                TextData broadcast = text;
                broadcast.receiver = std::stoi(uid);
                mq.pushToSendQueue(Message(broadcast));
            }
        }
    }

    void handleFile(const Message &msg)
    {
        auto &file = *static_cast<const FileData *>(msg.data.get());
        auto &connMgr = ConnectionMgr::getInstance().getIOConnections();
        int fd = connMgr.getFd(std::to_string(file.sender));

        if (fd == -1)
        {
            std::cerr << "File transfer failed: client fd not found" << std::endl;
            return;
        }

        if (file.action == FileAction::UPLOAD)
        {
            ThreadPool::getInstance().enqueue([this, fd, file]()
                                              { handleFileUpload(fd, file); });
        }
        else if (file.action == FileAction::DOWNLOAD)
        {
            ThreadPool::getInstance().enqueue([this, fd, file]()
                                              { handleFileDownload(fd, file); });
        }
    }

    void handleFileUpload(int fd, const FileData &file)
    {
        std::string fileName = file.filename.data();

        Socket clientSocket(fd);
        FileTransfer transfer(clientSocket);

        if (!transfer.receiveFile(fileName, file.filesize))
        {
            std::cerr << "Failed to receive file: " << fileName << std::endl;
            return;
        }

        std::cout << "File received and saved to: " << fileName << std::endl;

        sendFileNotification(file);
    }

    void handleFileDownload(int fd, const FileData &file)
    {
        std::string fileName = file.filename.data();

        Socket clientSocket(fd);
        FileTransfer transfer(clientSocket);

        if (!transfer.sendFile(fileName))
        {
            std::cerr << "Failed to send file: " << fileName << std::endl;
            return;
        }

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
        ss.read(notification.content.data(), notification.content.size() - 1);

        mq.pushToSendQueue(Message(notification));
    }

    MessageQueue &mq;
};

#endif // MSGHANDLER_HPP