#include "net/Socket.hpp"
#include <iostream>

int main()
{
    Socket server;
    server.initServer(9527);

    Socket client = server.accept();

    while (1)
    {
        std::string request;
        if (client.recv(request))
        {
            std::cout << "Received: " << request << std::endl;
            client.send("Hello, Client!");
        }
    }

    client.close();
}