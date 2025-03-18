#include <iostream>
#include <cstring>
#include "net/Pack.hpp"

// 枚举类型，用于区分消息类型
enum MessageType
{
    TYPE_FILE_INFO = 1,
    TYPE_CHAT_MESSAGE = 2
};

// 文件信息结构体
struct FileInfo
{
    char fileName[256]; // 文件名
    uint32_t fileSize;  // 文件大小
};

// 聊天消息结构体
struct ChatMessage
{
    char sender[32];   // 发送者
    char content[256]; // 消息内容
};

int main()
{
    // 测试文件信息
    FileInfo fileInfo;
    strcpy(fileInfo.fileName, "test.txt");
    fileInfo.fileSize = 1024;

    // 封包
    Pack filePack(TYPE_FILE_INFO, reinterpret_cast<const char *>(&fileInfo), sizeof(fileInfo));
    std::cout << "FileInfo Packet: ";
    filePack.dump();

    // 将封包对象转换为字符串
    std::string packetData = filePack.toString();

    // 解包
    Pack unpackedFilePack(packetData.c_str(), packetData.size());
    const FileInfo *unpackedFileInfo = reinterpret_cast<const FileInfo *>(unpackedFilePack.getData().data());

    // 验证数据
    std::cout << "Unpacked FileInfo: " << unpackedFileInfo->fileName << ", " << unpackedFileInfo->fileSize << " bytes" << std::endl;

    // 测试聊天消息
    ChatMessage chatMessage;
    strcpy(chatMessage.sender, "User1");
    strcpy(chatMessage.content, "Hello, how are you?");

    // 封包
    Pack chatPack(TYPE_CHAT_MESSAGE, reinterpret_cast<const char *>(&chatMessage), sizeof(chatMessage));
    std::cout << "ChatMessage Packet: ";
    chatPack.dump();

    // 将封包对象转换为字符串
    packetData = chatPack.toString();

    // 解包
    Pack unpackedChatPack(packetData.c_str(), packetData.size());
    const ChatMessage *unpackedChatMessage = reinterpret_cast<const ChatMessage *>(unpackedChatPack.getData().data());

    // 验证数据
    std::cout << "Unpacked ChatMessage: " << unpackedChatMessage->sender << ": " << unpackedChatMessage->content << std::endl;
}
