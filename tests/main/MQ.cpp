#include <iostream>
#include <thread>
#include "server/MQ.hpp"
#include "server/Message.hpp"

// 生产者线程：向接收队列添加消息
void producer()
{
    MessageQueue &mq = MessageQueue::getInstance();

    UserData user{1, "Alice", "password123", UserAction::HEARTBEAT};
    TextData text{1001, 1002, "Hello, World!"};
    FileData file{2001, 2002, "example.txt", 1024};

    mq.pushToRecvQueue(Message(user)); // 使用 Message 的构造函数
    mq.pushToRecvQueue(Message(text)); // 使用 Message 的构造函数
    mq.pushToRecvQueue(Message(file)); // 使用 Message 的构造函数

    std::cout << "Producer: Added messages to receive queue." << std::endl;
}

// 消费者线程：从接收队列取出消息
void consumer()
{
    MessageQueue &mq = MessageQueue::getInstance();

    while (true)
    {
        Message message = mq.popFromRecvQueue();
        switch (message.type)
        {
        case Message::Type::USER:
        {
            auto *user = static_cast<UserData *>(message.data.get());
            std::cout << "Consumer: Recvd UserData - Username: " << user->username.data() << std::endl;
            break;
        }
        case Message::Type::TEXT:
        {
            auto *text = static_cast<TextData *>(message.data.get());
            std::cout << "Consumer: Recvd TextData - Content: " << text->content.data() << std::endl;
            break;
        }
        case Message::Type::FILE:
        {
            auto *file = static_cast<FileData *>(message.data.get());
            std::cout << "Consumer: Recvd FileData - Filename: " << file->filename.data() << std::endl;
            break;
        }
        }

        if (mq.isRecvQueueEmpty())
        {
            break;
        }
    }
}

int main()
{
    std::thread producerThread(producer);
    std::thread consumerThread(consumer);

    producerThread.join();
    consumerThread.join();

    return 0;
}