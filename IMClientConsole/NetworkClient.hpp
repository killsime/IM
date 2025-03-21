#ifndef NETWORKCLIENT_HPP
#define NETWORKCLIENT_HPP

#include "Socket.hpp"
#include "Pack.hpp"
#include "Message.hpp"
#include "FileTransfer.hpp"
#include <thread>
#include <chrono>
#include <functional>
#include <atomic>

class NetworkClient
{
public:
	NetworkClient(const std::string& serverIP, uint16_t msgPort, uint16_t filePort)
		: msgClient(), fileClient(), serverIP(serverIP), msgPort(msgPort), filePort(filePort), running(false), globalUserId(0) {}

	~NetworkClient()
	{
		stop();
	}

	bool init()
	{
		if (!msgClient.initClient(serverIP, msgPort))
		{
			std::cerr << "Failed to connect to message server." << std::endl;
			return false;
		}

		return true;
	}

	void start()
	{
		running = true;
		receiveThread = std::thread(&NetworkClient::receiveMessages, this);
		heartbeatThread = std::thread(&NetworkClient::sendHeartbeat, this);
	}

	void stop()
	{
		running = false;
		if (receiveThread.joinable())
			receiveThread.join();
		if (heartbeatThread.joinable())
			heartbeatThread.join();
		msgClient.close();
		fileClient.close();
	}

	void sendTextMessage(const std::string& content, uint32_t receiver)
	{
		TextData textData{ globalUserId, receiver, {} };
		std::copy(content.begin(), content.end(), textData.content.data());
		textData.content[content.size()] = '\0';

		std::vector<char> data(reinterpret_cast<char*>(&textData), reinterpret_cast<char*>(&textData) + sizeof(textData));
		Pack textPack(2, data);
		msgClient.send(textPack.toByteStream());
	}

	void uploadFile(const std::string& fileName)
	{
		// 初始化文件传输的 Socket
		Socket fileClient;
		if (!fileClient.initClient(serverIP, filePort))
		{
			std::cerr << "Failed to connect to file server." << std::endl;
			return;
		}

		// 检查文件是否存在
		std::string filePath = "./send/" + fileName;
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
			globalUserId, // sender
			0,            // receiver (服务器)
			{},           // filename
			fileSize,     // filesize
			0,            // offset
			FileAction::UPLOAD // action
		};
		std::copy(fileName.begin(), fileName.end(), fileData.filename.data());
		fileData.filename[fileName.size()] = '\0'; // 确保字符串以 '\0' 结尾

		// 封装为 Pack 并发送请求
		std::vector<char> data(reinterpret_cast<char*>(&fileData), reinterpret_cast<char*>(&fileData) + sizeof(fileData));
		Pack requestPack(3, data);
		if (!fileClient.send(requestPack.toByteStream()))
		{
			std::cerr << "Failed to send upload request." << std::endl;
			return;
		}

		// 初始化文件传输
		FileTransfer transfer(fileClient);
		transfer.setRepoPath("./send");

		// 发送文件
		if (!transfer.sendFile(fileName))
		{
			std::cerr << "Failed to send file: " << fileName << std::endl;
			return;
		}

		std::cout << "File uploaded successfully: " << fileName << std::endl;
		fileClient.close(); // 传输完成后关闭 Socket
	}

	void downloadFile(const std::string& fileName)
	{
		// 初始化文件传输的 Socket
		Socket fileClient;
		if (!fileClient.initClient(serverIP, filePort))
		{
			std::cerr << "Failed to connect to file server." << std::endl;
			return;
		}

		// 构造文件下载请求
		FileData fileData{
			globalUserId, // sender
			0,           // receiver (服务器)
			{},          // filename
			0,           // filesize
			0,           // offset
			FileAction::DOWNLOAD // action
		};
		std::copy(fileName.begin(), fileName.end(), fileData.filename.data());
		fileData.filename[fileName.size()] = '\0'; // 确保字符串以 '\0' 结尾

		// 封装为 Pack 并发送请求
		std::vector<char> data(reinterpret_cast<char*>(&fileData), reinterpret_cast<char*>(&fileData) + sizeof(fileData));
		Pack requestPack(3, data);
		if (!fileClient.send(requestPack.toByteStream()))
		{
			std::cerr << "Failed to send download request." << std::endl;
			return;
		}

		// 初始化文件传输
		FileTransfer transfer(fileClient);
		transfer.setRepoPath("./recv");

		// 接收文件
		if (!transfer.receiveFile(fileName))
		{
			std::cerr << "Failed to receive file: " << fileName << std::endl;
			return;
		}

		std::cout << "File downloaded successfully: " << fileName << std::endl;
		fileClient.close(); // 传输完成后关闭 Socket
	}
	bool login(uint32_t userId)
	{
		globalUserId = userId;
		UserData loginData{ globalUserId, {}, {}, UserAction::LOGIN };
		std::vector<char> data(reinterpret_cast<char*>(&loginData), reinterpret_cast<char*>(&loginData) + sizeof(loginData));
		Pack loginPack(1, data);

		if (msgClient.send(loginPack.toByteStream()))
		{
			std::cout << "Login request sent." << std::endl;
		}
		else
		{
			std::cerr << "Failed to send login request." << std::endl;
			return false;
		}

		// 等待登录响应
		std::this_thread::sleep_for(std::chrono::seconds(1));
		return true;
	}


	void setMessageCallback(std::function<void(const Message&)> callback)
	{
		messageCallback = callback;
	}

private:
	void receiveMessages()
	{
		while (running)
		{
			std::vector<char> response;
			if (msgClient.recv(response))
			{
				try
				{
					Pack pack(response);
					uint16_t type = pack.getType();
					const std::vector<char>& data = pack.getData();

					Message message;
					switch (type)
					{
					case 1: // UserData
						message = Message(*reinterpret_cast<const UserData*>(data.data()));
						break;
					case 2: // TextData
						message = Message(*reinterpret_cast<const TextData*>(data.data()));
						break;
					case 3: // FileData
						message = Message(*reinterpret_cast<const FileData*>(data.data()));
						break;
					default:
						std::cerr << "Unknown message type: " << type << std::endl;
						continue;
					}

					if (messageCallback)
						messageCallback(message);
				}
				catch (const std::exception& e)
				{
					std::cerr << "Failed to unpack message: " << e.what() << std::endl;
				}
			}
			else
			{
				std::cerr << "Connection closed by server." << std::endl;
				running = false;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	void sendHeartbeat()
	{
		while (running)
		{
			
			UserData heartbeat{ globalUserId, {}, {}, UserAction::HEARTBEAT };
			std::vector<char> data(reinterpret_cast<char*>(&heartbeat), reinterpret_cast<char*>(&heartbeat) + sizeof(heartbeat));

			Pack heartbeatPack(1, data); 
			if (!msgClient.send(heartbeatPack.toByteStream()))
			{
				std::cerr << "Failed to send heartbeat." << std::endl;
				running = false; // 如果发送失败，停止心跳线程
				break;
			}

			std::this_thread::sleep_for(std::chrono::seconds(8));
		}
	}



	uint32_t globalUserId;
	Socket msgClient;
	Socket fileClient;
	std::string serverIP;
	uint16_t msgPort;
	uint16_t filePort;
	std::atomic<bool> running;
	std::thread receiveThread;
	std::thread heartbeatThread;
	std::function<void(const Message&)> messageCallback;
};

#endif // NETWORKCLIENT_HPP