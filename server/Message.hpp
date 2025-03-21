#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <memory>
#include <cstdint>
#include <array>
#include <iostream>
#include <array>
#include <cstdint>

enum class UserAction : uint8_t
{
    HEARTBEAT = 0, // 心跳包
    LOGIN = 1,     // 登录
    LOGOUT = 2,    // 登出
    REGISTER = 3   // 注册
};
struct UserData
{
    uint32_t uid;
    std::array<char, 32> username;
    std::array<char, 32> password;
    UserAction action;
};

enum class TextType : uint8_t
{
    PRIVATE = 0, // 私发
    GROUP = 1    // 群发
};
struct TextData
{
    uint32_t sender;               // 发送者UID
    uint32_t receiver;             // 接收者UID（私发时为单个用户UID，群发时为群组ID）
    std::array<char, 512> content; // 消息内容
    TextType type;                 // 消息类型：私发或群发
};

enum class FileAction : uint8_t
{
    UPLOAD = 0,  // 上传
    DOWNLOAD = 1 // 下载
};
struct FileData
{
    uint32_t sender;                // 发送者UID
    uint32_t receiver;              // 接收者UID（上传时为服务器UID，下载时为请求者UID）
    std::array<char, 256> filename; // 文件名
    uint64_t filesize;              // 文件大小
    uint64_t offset;                // 文件偏移量（用于断点续传）
    FileAction action;              // 文件操作：上传或下载
};

class Message
{
public:
    enum class Type
    {
        USER,
        TEXT,
        FILE
    };
    Type type;
    std::unique_ptr<void, void (*)(void *)> data;
    Message() : type(Type::USER), data(nullptr, [](void *ptr) {}) {}

    // 构造函数
    Message(UserData user) : type(Type::USER), data(new UserData(user), [](void *ptr)
                                                    { delete static_cast<UserData *>(ptr); }) {}
    Message(TextData text) : type(Type::TEXT), data(new TextData(text), [](void *ptr)
                                                    { delete static_cast<TextData *>(ptr); }) {}
    Message(FileData file) : type(Type::FILE), data(new FileData(file), [](void *ptr)
                                                    { delete static_cast<FileData *>(ptr); }) {}

    // 删除拷贝构造函数和拷贝赋值运算符
    Message(const Message &) = delete;
    Message &operator=(const Message &) = delete;

    // 移动构造函数
    Message(Message &&other) noexcept
        : type(other.type), data(std::move(other.data))
    {
        other.type = Type::USER; // 重置 other 的状态
    }

    // 移动赋值运算符
    Message &operator=(Message &&other) noexcept
    {
        if (this != &other)
        {
            type = other.type;
            data = std::move(other.data);
            other.type = Type::USER; // 重置 other 的状态
        }
        return *this;
    }

    void print() const
    {
        switch (type)
        {
        case Type::USER:
        {
            auto *user = static_cast<UserData *>(data.get());
            std::cout << "UserData: " << user->username.data() << std::endl;
            break;
        }
        case Type::TEXT:
        {
            auto *text = static_cast<TextData *>(data.get());
            std::cout << "TextData: " << text->content.data() << std::endl;
            break;
        }
        case Type::FILE:
        {
            auto *file = static_cast<FileData *>(data.get());
            std::cout << "FileData: " << file->filename.data() << std::endl;
            break;
        }
        }
    }
};

#endif // MESSAGE_HPP