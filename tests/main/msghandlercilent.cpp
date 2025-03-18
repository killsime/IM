// #include "Socket.hpp"
// #include "Pack.hpp"
// #include "Message.hpp"
// #include <iostream>
// #include <thread>
// #include <string>
// #include <chrono>
// #include <algorithm> // for std::copy

// #define MSG_PORT 9527

// // 全局变量
// Socket client;
// bool isLoggedIn = false;
// uint32_t globalUserId = 0; // 全局用户ID

// // 接收服务器消息的线程
// void receiveMessages()
// {
//     while (true)
//     {
//         std::string response;
//         if (client.recv(response))
//         {
//             try
//             {
//                 // 解包
//                 Pack pack(response.data(), response.size());
//                 uint16_t type = pack.getType();
//                 const std::string &data = pack.getData();

//                 // 根据消息类型处理
//                 switch (type)
//                 {
//                 case 1:
//                 { // UserData
//                     UserData *user = reinterpret_cast<UserData *>(const_cast<char *>(data.data()));
//                     std::cout << "Received UserData: UID=" << user->uid
//                               << ", Action=" << static_cast<int>(user->action) << std::endl;
//                     break;
//                 }
//                 case 2:
//                 { // TextData
//                     TextData *text = reinterpret_cast<TextData *>(const_cast<char *>(data.data()));
//                     std::cout << "Received TextData: Sender=" << text->sender
//                               << ", Receiver=" << text->receiver
//                               << ", Content=" << text->content.data() << std::endl;
//                     break;
//                 }
//                 case 3:
//                 { // FileData
//                     FileData *file = reinterpret_cast<FileData *>(const_cast<char *>(data.data()));
//                     std::cout << "Received FileData: Sender=" << file->sender
//                               << ", Receiver=" << file->receiver
//                               << ", Filename=" << file->filename.data() << std::endl;
//                     break;
//                 }
//                 default:
//                     std::cerr << "Unknown message type: " << type << std::endl;
//                     break;
//                 }
//             }
//             catch (const std::exception &e)
//             {
//                 std::cerr << "Failed to unpack message: " << e.what() << std::endl;
//             }
//         }
//         else
//         {
//             std::cerr << "Connection closed by server." << std::endl;
//             exit(1);
//         }
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     }
// }

// // 登录功能
// void login()
// {
//     std::cout << "Enter your user ID: ";
//     std::cin >> globalUserId;

//     // 构造登录消息
//     UserData loginData{globalUserId, {}, {}, UserAction::LOGIN};
//     Pack loginPack(1, reinterpret_cast<const char *>(&loginData), sizeof(loginData));

//     // 发送登录消息
//     if (client.send(loginPack.toString()))
//     {
//         std::cout << "Login request sent." << std::endl;
//     }
//     else
//     {
//         std::cerr << "Failed to send login request." << std::endl;
//         exit(1);
//     }

//     // 等待登录响应
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//     isLoggedIn = true;
// }

// // 聊天功能
// void chat()
// {
//     std::string input;
//     std::cin.ignore(); // 忽略之前输入留下的换行符
//     while (true)
//     {
//         std::cout << "Enter your message (or 'exit' to quit): ";
//         std::getline(std::cin, input);

//         if (input == "exit")
//         {
//             break;
//         }

//         // 构造文本消息
//         TextData textData{globalUserId, 0, {}}; // 发送给服务器（receiver=0）
//         std::copy(input.begin(), input.end(), textData.content.data());
//         textData.content[input.size()] = '\0'; // 确保字符串以 '\0' 结尾
//         Pack textPack(2, reinterpret_cast<const char *>(&textData), sizeof(textData));

//         // 发送消息
//         if (client.send(textPack.toString()))
//         {
//             std::cout << "Message sent." << std::endl;
//         }
//         else
//         {
//             std::cerr << "Failed to send message." << std::endl;
//             break;
//         }
//     }
// }

// int main()
// {
//     // 初始化客户端Socket
//     if (!client.initClient("127.0.0.1", MSG_PORT))
//     {
//         std::cerr << "Failed to connect to server." << std::endl;
//         return 1;
//     }

//     // 启动接收消息线程
//     std::thread(receiveMessages).detach();

//     // 登录
//     login();

//     // 开始聊天
//     if (isLoggedIn)
//     {
//         std::cout << "Logged in successfully. Start chatting!" << std::endl;
//         chat();
//     }

//     // 关闭连接
//     client.close();
//     std::cout << "Connection closed." << std::endl;

//     return 0;
// }