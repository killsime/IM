#ifndef CONSOLEUI_HPP
#define CONSOLEUI_HPP

#include "NetworkClient.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <atomic>

class ConsoleUI
{
public:
	ConsoleUI(NetworkClient& networkClient) : networkClient(networkClient), running(false){}

	void start()
	{
		// 设置消息回调
		networkClient.setMessageCallback([this](const Message& message)
			{ handleMessage(message); });

		// 启动网络客户端
		networkClient.start();

		if (login()) // 确保登录成功
		{
			running = true;
			showMenu();

		}
		else
		{
			std::cerr << "Login failed. Exiting..." << std::endl;
			stop();
		}
	}

	void stop()
	{
		running = false;
		networkClient.stop();
	}

	bool login()
	{
		uint32_t globalUserId;
		clearScreen();
		std::cout << "Enter your user ID: ";
		std::cin >> globalUserId;

		return networkClient.login(globalUserId);
	}

private:
	void clearScreen()
	{
#ifdef _WIN32
		system("cls");
#else
		system("clear");
#endif
	}

	void handleMessage(const Message& message)
	{
		switch (message.type)
		{
		case Message::Type::USER:
		{
			auto* user = static_cast<UserData*>(message.data.get());
			std::cout << "Received UserData: UID=" << user->uid << ", Action=" << static_cast<int>(user->action) << std::endl;
			break;
		}
		case Message::Type::TEXT:
		{
			auto* text = static_cast<TextData*>(message.data.get());
			std::cout << text->sender << ": " << text->content.data() << std::endl;
			break;
		}
		case Message::Type::FILE:
		{
			auto* file = static_cast<FileData*>(message.data.get());
			std::cout << "Received FileData: Sender=" << file->sender << ", Receiver=" << file->receiver << ", Filename=" << file->filename.data() << std::endl;
			break;
		}
		default:
			break;
		}
	}

	void showMenu()
	{
		while (running)
		{
			clearScreen();
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
				stop();
				return;
			default:
				std::cerr << "Invalid choice. Please try again." << std::endl;
				break;
			}
		}
	}

	void chat()
	{
		clearScreen();
		std::string input;
		std::cin.ignore();
		while (running)
		{
			std::cout << "Enter your message (or 'exit' to quit chat): ";
			std::getline(std::cin, input);

			if (input == "exit")
				break;

			networkClient.sendTextMessage(input, 0); // 0 表示发送给服务器
		}
	}

	void uploadFile()
	{
		clearScreen();
		std::string fileName = "testfile";
		networkClient.uploadFile(fileName);
		std::cout << "File uploaded successfully: " << fileName << std::endl;
		system("pause");
	}

	void downloadFile()
	{
		clearScreen();
		std::string fileName = "testfile";
		networkClient.downloadFile(fileName);
		std::cout << "File downloaded successfully: " << fileName << std::endl;
		system("pause");
	}

	NetworkClient& networkClient;
	std::atomic<bool> running;
};

#endif // CONSOLEUI_HPP