#include "net/Socket.hpp"
#include "net/Transfer/FileTransfer.hpp"
#include <iostream>
#include <filesystem> // C++17

// 确保目录存在
void ensureDirectoryExists(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directories(path);
    }
}

int main() {
    // 初始化服务端 Socket
    Socket socket;
    socket.initServer(9527);
    // 确保 ./data 目录存在
    ensureDirectoryExists("./data");

    // 循环处理客户端连接
    while (true) {
        std::cout << "Waiting for client connection..." << std::endl;

        // 接受客户端连接
        Socket client = socket.accept();
        if (client.getFd() == INVALID_SOCKET) {
            std::cerr << "Failed to accept client connection." << std::endl;
            continue; // 继续等待下一个客户端
        }

        std::cout << "Client connected." << std::endl;

        // 初始化文件传输
        FileTransfer transfer(client);

        // 接收文件
        std::string receivedFilePath = "./data/received_testfile.txt"; // 保存到 ./data 目录
        if (!transfer.receiveFile(receivedFilePath)) {
            std::cerr << "Failed to receive file." << std::endl;
            client.close();
            continue; // 继续等待下一个客户端
        }
        std::cout << "File received and saved to: " << receivedFilePath << std::endl;

        // 将接收到的文件发送回客户端
        if (!transfer.sendFile(receivedFilePath)) {
            std::cerr << "Failed to send file back to client." << std::endl;
            client.close();
            continue; // 继续等待下一个客户端
        }
        std::cout << "File sent back to client successfully." << std::endl;

        // 关闭当前客户端连接
        client.close();
        std::cout << "Client disconnected." << std::endl;
    }

    return 0;
}