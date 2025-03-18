// #include "Socket.hpp"
// #include "Pack.hpp"
// #include "Message.hpp"
// #include <iostream>
// #include <thread>
// #include <chrono>

// #define MSG_PORT 9527
// #define FILE_PORT 9528

// int main()
// {
//     // 初始化客户端Socket
//     Socket client;
//     client.initClient("127.0.0.1", MSG_PORT);

//     // 发送登录消息
//     UserData loginData{1001, "Alice", "password123", UserAction::LOGIN};
//     Pack loginPack(1, reinterpret_cast<const char *>(&loginData), sizeof(loginData));
//     client.send(loginPack.toString());
//     std::cout << "Sent login message" << std::endl;

//     // 等待1秒
//     std::this_thread::sleep_for(std::chrono::seconds(1));

//     // 发送文本消息
//     TextData textData{1001, 1002, "Hello, Server!"};
//     Pack textPack(2, reinterpret_cast<const char *>(&textData), sizeof(textData));
//     client.send(textPack.toString());
//     std::cout << "Sent text message" << std::endl;

//     // 保持连接以接收服务器响应
//     while (true)
//     {
//         std::string response;
//         if (client.recv(response))
//         {
//             std::cout << "Received from server: " << response << std::endl;
//         }
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     }

//     return 0;
// }