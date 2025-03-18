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