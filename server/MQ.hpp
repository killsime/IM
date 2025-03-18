#ifndef MQ_HPP
#define MQ_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include "Message.hpp"

class MessageQueue
{
private:
    std::queue<Message> receiveQueue;  // 接收队列
    std::queue<Message> sendQueue;     // 发送队列
    std::mutex receiveMutex;           // 接收队列的互斥锁
    std::mutex sendMutex;              // 发送队列的互斥锁
    std::condition_variable receiveCV; // 接收队列的条件变量
    std::condition_variable sendCV;    // 发送队列的条件变量

    // 单例模式：私有构造函数
    MessageQueue() = default;

public:
    // 删除拷贝构造函数和赋值运算符
    MessageQueue(const MessageQueue &) = delete;
    MessageQueue &operator=(const MessageQueue &) = delete;

    // 获取单例实例
    static MessageQueue &getInstance()
    {
        static MessageQueue instance;
        return instance;
    }

    // 将消息推入接收队列
    void pushToReceiveQueue(Message &&message)
    {
        std::unique_lock<std::mutex> lock(receiveMutex);
        receiveQueue.push(std::move(message)); // 使用 std::move
        receiveCV.notify_one();
    }

    // 从接收队列取出消息
    Message popFromReceiveQueue()
    {
        std::unique_lock<std::mutex> lock(receiveMutex);
        receiveCV.wait(lock, [this]
                       { return !receiveQueue.empty(); }); // 等待队列不为空
        Message message = std::move(receiveQueue.front()); // 使用 std::move
        receiveQueue.pop();
        return message;
    }

    // 将消息推入发送队列
    void pushToSendQueue(Message &&message)
    {
        std::unique_lock<std::mutex> lock(sendMutex);
        sendQueue.push(std::move(message)); // 使用 std::move
        sendCV.notify_one();
    }

    // 从发送队列取出消息
    Message popFromSendQueue()
    {
        std::unique_lock<std::mutex> lock(sendMutex);
        sendCV.wait(lock, [this]
                    { return !sendQueue.empty(); });    // 等待队列不为空
        Message message = std::move(sendQueue.front()); // 使用 std::move
        sendQueue.pop();
        return message;
    }

    // 检查接收队列是否为空
    bool isReceiveQueueEmpty()
    {
        std::unique_lock<std::mutex> lock(receiveMutex);
        return receiveQueue.empty();
    }

    // 检查发送队列是否为空
    bool isSendQueueEmpty()
    {
        std::unique_lock<std::mutex> lock(sendMutex);
        return sendQueue.empty();
    }
};

#endif // MQ_HPP