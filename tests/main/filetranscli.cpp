// 主要是用于演示tcp的拥塞,导致的数据丢包,这是windows客户端主程序

#include "Socket.hpp"
#include "Pack.hpp"
#include "Message.hpp"
#include "FileTransfer.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <chrono>
#include <algorithm> // for std::copy

#define MSG_PORT 9527
#define FILE_PORT 9528

// 全局变量
Socket msgClient; // 用于文本传输的客户端
bool isLoggedIn = false;
uint32_t globalUserId = 0; // 全局用户ID

void receiveMessages()
{
    while (true)
    {
        std::vector<char> response;
        if (msgClient.recv(response))
        {
            try
            {
                // 解包
                Pack pack(response);
                uint16_t type = pack.getType();
                const std::vector<char> &data = pack.getData();

                // 根据消息类型处理
                switch (type)
                {
                case 1:
                { // UserData
                    const UserData *user = reinterpret_cast<const UserData *>(data.data());
                    std::cout << "Received UserData: UID=" << user->uid
                              << ", Action=" << static_cast<int>(user->action) << std::endl;
                    break;
                }
                case 2:
                { // TextData
                    const TextData *text = reinterpret_cast<const TextData *>(data.data());
                    std::cout << "Received TextData: Sender=" << text->sender
                              << ", Receiver=" << text->receiver
                              << ", Content=" << text->content.data() << std::endl;
                    break;
                }
                case 3:
                { // FileData
                    const FileData *file = reinterpret_cast<const FileData *>(data.data());
                    std::cout << "Received FileData: Sender=" << file->sender
                              << ", Receiver=" << file->receiver
                              << ", Filename=" << file->filename.data() << std::endl;
                    break;
                }
                default:
                    std::cerr << "Unknown message type: " << type << std::endl;
                    break;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Failed to unpack message: " << e.what() << std::endl;
            }
        }
        else
        {
            std::cerr << "Connection closed by server." << std::endl;
            exit(1);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
// 登录功能
void login()
{
    std::cout << "Enter your user ID: ";
    std::cin >> globalUserId;

    // 构造登录消息
    UserData loginData{globalUserId, {}, {}, UserAction::LOGIN};
    std::vector<char> data(reinterpret_cast<char *>(&loginData), reinterpret_cast<char *>(&loginData) + sizeof(loginData));
    Pack loginPack(1, data);

    // 发送登录消息
    if (msgClient.send(loginPack.toByteStream()))
    {
        std::cout << "Login request sent." << std::endl;
    }
    else
    {
        std::cerr << "Failed to send login request." << std::endl;
        exit(1);
    }

    // 等待登录响应
    std::this_thread::sleep_for(std::chrono::seconds(1));
    isLoggedIn = true;
}

// 聊天功能
void chat()
{
    std::string input;
    std::cin.ignore(); // 忽略之前输入留下的换行符
    while (true)
    {
        std::cout << "Enter your message (or 'exit' to quit chat): ";
        std::getline(std::cin, input);

        if (input == "exit")
        {
            break;
        }

        // 构造文本消息
        TextData textData{globalUserId, 0, {}}; // 发送给服务器（receiver=0）
        std::copy(input.begin(), input.end(), textData.content.data());
        textData.content[input.size()] = '\0'; // 确保字符串以 '\0' 结尾
        std::vector<char> data(reinterpret_cast<char *>(&textData), reinterpret_cast<char *>(&textData) + sizeof(textData));
        Pack textPack(2, data);

        // 发送消息
        if (msgClient.send(textPack.toByteStream()))
        {
            std::cout << "Message sent." << std::endl;
        }
        else
        {
            std::cerr << "Failed to send message." << std::endl;
            break;
        }
    }
}

// 上传文件功能
void uploadFile()
{
    Socket fileClient; // 用于文件传输的客户端

    if (!fileClient.initClient("127.0.0.1", FILE_PORT))
    {
        std::cerr << "Failed to connect to file server." << std::endl;
        return;
    }

    std::string fileName = "testfile";           // 文件名
    std::string filePath = "./send/" + fileName; // 文件路径

    // 检查文件是否存在
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return;
    }

    // 获取文件大小
    uint64_t fileSize = file.tellg();
    file.close();

    // 构造文件上传请求
    FileData fileData{
        globalUserId,      // sender
        0,                 // receiver (服务器)
        {},                // filename
        fileSize,          // filesize
        0,                 // offset
        FileAction::UPLOAD // action
    };
    std::copy(fileName.begin(), fileName.end(), fileData.filename.data());
    fileData.filename[fileName.size()] = '\0'; // 确保字符串以 '\0' 结尾

    // 封装为 Pack 并发送请求
    std::vector<char> data(reinterpret_cast<char *>(&fileData), reinterpret_cast<char *>(&fileData) + sizeof(fileData));
    Pack requestPack(3, data);
    if (!fileClient.send(requestPack.toByteStream()))
    {
        std::cerr << "Failed to send upload request." << std::endl;
        return;
    }

    // 初始化文件传输
    FileTransfer transfer(fileClient);
    transfer.setRepoPath("./send"); // 设置上传路径

    // 发送文件
    if (!transfer.sendFile(fileName))
    {
        std::cerr << "Failed to send file: " << fileName << std::endl;
        return;
    }

    std::cout << "File uploaded successfully: " << fileName << std::endl;
}

// 下载文件功能
void downloadFile()
{
    Socket fileClient; // 用于文件传输的客户端

    if (!fileClient.initClient("127.0.0.1", FILE_PORT))
    {
        std::cerr << "Failed to connect to file server." << std::endl;
        return;
    }

    std::string fileName = "testfile"; // 文件名

    // 构造文件下载请求
    FileData fileData{
        globalUserId,        // sender
        0,                   // receiver (服务器)
        {},                  // filename
        0,                   // filesize
        0,                   // offset
        FileAction::DOWNLOAD // action
    };
    std::copy(fileName.begin(), fileName.end(), fileData.filename.data());
    fileData.filename[fileName.size()] = '\0'; // 确保字符串以 '\0' 结尾

    // 封装为 Pack 并发送请求
    std::vector<char> data(reinterpret_cast<char *>(&fileData), reinterpret_cast<char *>(&fileData) + sizeof(fileData));
    Pack requestPack(3, data);
    if (!fileClient.send(requestPack.toByteStream()))
    {
        std::cerr << "Failed to send download request." << std::endl;
        return;
    }

    // 初始化文件传输
    FileTransfer transfer(fileClient);
    transfer.setRepoPath("./recv"); // 设置下载路径

    // 接收文件
    if (!transfer.receiveFile(fileName))
    {
        std::cerr << "Failed to receive file from server." << std::endl;
        return;
    }
    std::cout << "File downloaded successfully: " << fileName << std::endl;
}

// 显示菜单
void showMenu()
{
    while (true)
    {
        std::cout << "\n===== Menu =====\n";
        std::cout << "1. Enter Chat\n";
        std::cout << "2. Upload File\n";
        std::cout << "3. Download File\n";
        std::cout << "4. Exit\n";
        std::cout << "Enter your choice: ";

        int choice;
        std::cin >> choice;

        switch (choice)
        {
        case 1:
            chat();
            break;
        case 2:
            uploadFile();
            break;
        case 3:
            downloadFile();
            break;
        case 4:
            std::cout << "Exiting..." << std::endl;
            return;
        default:
            std::cerr << "Invalid choice. Please try again." << std::endl;
            break;
        }
    }
}

int main()
{
    // 初始化客户端Socket
    if (!msgClient.initClient("127.0.0.1", MSG_PORT))
    {
        std::cerr << "Failed to connect to message server." << std::endl;
        return 1;
    }

    // 启动接收消息线程
    std::thread(receiveMessages).detach();

    // 登录
    login();

    // 显示菜单
    if (isLoggedIn)
    {
        showMenu();
    }

    // 关闭连接
    msgClient.close();
    std::cout << "Connection closed." << std::endl;

    return 0;
}