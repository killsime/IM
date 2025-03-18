#include "EventLoop.hpp"
#include "server/MQ.hpp"
#include "net/ConnectionMgr.hpp"
#include <iostream>

int main()
{
    // 初始化EventLoop
    EventLoop eventLoop;
    if (!eventLoop.init())
    {
        std::cerr << "Failed to initialize EventLoop" << std::endl;
        return 1;
    }

    // 启动消息处理线程
    std::thread([&]()
                {
        MessageQueue &mq = MessageQueue::getInstance();
        while (true) {
            if (!mq.isRecvQueueEmpty()) {
                Message message = mq.popFromRecvQueue();
                switch (message.type) {
                    case Message::Type::USER: {
                        auto *user = static_cast<UserData *>(message.data.get());
                        std::cout << "Received UserData: UID=" << user->uid << ", Action=" << static_cast<int>(user->action) << std::endl;
                        break;
                    }
                    case Message::Type::TEXT: {
                        auto *text = static_cast<TextData *>(message.data.get());
                        std::cout << "Received TextData: Sender=" << text->sender << ", Receiver=" << text->receiver << ", Content=" << text->content.data() << std::endl;
                        break;
                    }
                    case Message::Type::FILE: {
                        auto *file = static_cast<FileData *>(message.data.get());
                        std::cout << "Received FileData: Sender=" << file->sender << ", Receiver=" << file->receiver << ", Filename=" << file->filename.data() << std::endl;
                        break;
                    }
                }

                // 打印当前在线UID
                auto &connections = ConnectionMgr::getInstance().getTextConnections();
                std::cout << "Online UIDs: ";
                for (const auto &[uid, _] : connections.getConnections()) {
                    std::cout << uid << " ";
                }
                std::cout << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 避免忙等待
        } })
        .detach();

    // 运行EventLoop
    eventLoop.run();
    return 0;
}