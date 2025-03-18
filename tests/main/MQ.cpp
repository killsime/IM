#include <iostream>
#include <thread>
#include "server/MQ.hpp"
#include "server/Message.hpp"

// 生产者线程：向接收队列添加消息
void producer()
{
    MessageQueue &mq = MessageQueue::getInstance();

    UserData user{1, "Alice", "password123", 0};
    TextData text{1001, 1002, "Hello, World!"};
    FileData file{2001, 2002, "example.txt", 1024};

    mq.pushToReceiveQueue(Message(user)); // 使用 Message 的构造函数
    mq.pushToReceiveQueue(Message(text)); // 使用 Message 的构造函数
    mq.pushToReceiveQueue(Message(file)); // 使用 Message 的构造函数

    std::cout << "Producer: Added messages to receive queue." << std::endl;
}

// 消费者线程：从接收队列取出消息
void consumer()
{
    MessageQueue &mq = MessageQueue::getInstance();

    while (true)
    {
        Message message = mq.popFromReceiveQueue();
        switch (message.type)
        {
        case Message::Type::USER:
        {
            auto *user = static_cast<UserData *>(message.data.get());
            std::cout << "Consumer: Received UserData - Username: " << user->username.data() << std::endl;
            break;
        }
        case Message::Type::TEXT:
        {
            auto *text = static_cast<TextData *>(message.data.get());
            std::cout << "Consumer: Received TextData - Content: " << text->content.data() << std::endl;
            break;
        }
        case Message::Type::FILE:
        {
            auto *file = static_cast<FileData *>(message.data.get());
            std::cout << "Consumer: Received FileData - Filename: " << file->filename.data() << std::endl;
            break;
        }
        }

        if (mq.isReceiveQueueEmpty())
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