#ifndef MSGHANDLER_HPP
#define MSGHANDLER_HPP

#include "MQ.hpp"
#include "Message.hpp"
#include "../net/ConnectionMgr.hpp"
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

        // 构造文件通知消息
        std::stringstream ss;
        ss << "[文件] " << file.filename.data()
           << " (" << file.filesize << "字节)";

        TextData notification{
            file.sender,
            file.receiver,
            {},
            file.receiver == 0 ? TextType::GROUP : TextType::PRIVATE};
        ss.read(notification.content.data(), notification.content.size() - 1);

        mq.pushToSendQueue(Message(notification));
    }

    MessageQueue &mq;
};

#endif // MSGHANDLER_HPP