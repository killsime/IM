#include "net/Socket.hpp"
#include "net/Pack.hpp"
#include <iostream>

int main()
{
    Socket server;
    server.initServer(9527);

    Socket client = server.accept();
    // 接收数据

    std::string receivedData;
    if (client.recv(receivedData))
    {

        try
        {
            std::cout << "Received data: " << receivedData << std::endl;

            // 解包
            Pack unpackedPack(receivedData.c_str(), receivedData.size());
            unpackedPack.dump();

            // 解析数据
            if (unpackedPack.getType() == 1)
            {
                std::cout << "Received message: " << unpackedPack.getData() << std::endl;
            }
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    server.close();
    return 0;
}

// #include "Socket.hpp"
// #include "Pack.hpp"
// #include <iostream>

// int main()
// {
//     Socket client;
//     client.initClient("127.0.0.1", 9527);

//     // 构造数据
//     std::string message = "Hello, Server!";
//     Pack pack(1, message.c_str(), message.size());

//     // 将封包对象转换为字符串
//     std::string packetData = pack.toString();

//     // 发送数据
//     if (client.send(packetData))
//     {
//         std::cout << "Packet sent successfully!" << std::endl;
//     }
//     else
//     {
//         std::cerr << "Failed to send packet!" << std::endl;
//     }
//     client.close();
//     return 0;
// }