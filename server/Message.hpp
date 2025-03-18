#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <memory>
#include <cstdint>
#include <array>
#include <iostream>

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

struct TextData
{
    uint32_t sender;
    uint32_t receiver;
    std::array<char, 512> content;
};

struct FileData
{
    uint32_t sender;
    uint32_t receiver;
    std::array<char, 256> filename;
    uint64_t filesize;
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