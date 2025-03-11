#ifndef FILETRANSFER_HPP
#define FILETRANSFER_HPP

#include "../Socket.hpp" // 引用 Socket 类
#include <fstream>
#include <string>
#include <cstdio> // for printf
#include <cstring> // for memcpy
#include <iomanip> // for std::setw and std::setprecision
#include <iostream> // for std::cout

// 定义常量
#define CHUNK_SIZE 1024*1024        // 每个分片的大小（1MB）,分配在堆区
#define MAX_RETRIES 3          // 最大重试次数

class FileTransfer {
public:
	// 构造函数
	FileTransfer(Socket& socket) : socket_(socket), totalBytes_(0), transferredBytes_(0) {
		socket_.setBufferSize(CHUNK_SIZE);
	}

	bool sendFile(const std::string& filePath, uint64_t offset = 0) {
		std::ifstream file(filePath, std::ios::binary | std::ios::ate);
		if (!file.is_open()) {
			printf("Failed to open file: %s\n", filePath.c_str());
			return false;
		}

		// 获取文件大小
		totalBytes_ = file.tellg();
		file.seekg(offset, std::ios::beg);

		// 读取文件并分片发送
		char* buffer = new char[CHUNK_SIZE]; // 动态分配内存
		while (!file.eof()) {
			file.read(buffer, CHUNK_SIZE);
			std::streamsize bytesRead = file.gcount();

			// 发送分片
			if (!sendChunk(buffer, bytesRead)) {
				printf("Failed to send file chunk.\n");
				file.close();
				delete[] buffer; // 释放内存
				return false;
			}

			// 更新进度
			transferredBytes_ += bytesRead;
			printProgress();
		}

		// 发送结束标志（空字符串）
		if (!sendChunk("", 0)) {
			printf("Failed to send end marker.\n");
			file.close();
			delete[] buffer; // 释放内存
			return false;
		}

		delete[] buffer;
		file.close();
		printf("\nFile sent successfully.\n");
		return true;
	}

	bool receiveFile(const std::string& filePath, uint64_t offset = 0) {
		std::ofstream file(filePath, std::ios::binary | std::ios::trunc); // 使用 trunc 模式
		if (!file.is_open()) {
			printf("Failed to open file: %s\n", filePath.c_str());
			return false;
		}

		// 获取文件大小（如果已知）
		totalBytes_ = getFileSize(filePath) + offset;

		// 接收分片并写入文件
		char* buffer = new char[CHUNK_SIZE]; // 动态分配内存
		while (true) {
			std::streamsize bytesReceived = receiveChunk(buffer, CHUNK_SIZE);
			if (bytesReceived == 0) {
				printf("\nTransfer complete.\n");
				// 传输完成
				break;
			}
			else if (bytesReceived == -1) {
				printf("Failed to receive file chunk.\n");
				file.close();
				delete[] buffer; // 释放内存
				return false;
			}

			file.write(buffer, bytesReceived);

			// 更新进度
			transferredBytes_ += bytesReceived;
			printProgress();
		}

		delete[] buffer;
		file.close();
		printf("\nFile received successfully.\n");
		return true;
	}


	// 获取当前进度（0.0 到 1.0）
	double getProgress() const {
		if (totalBytes_ == 0) return 0.0;
		return static_cast<double>(transferredBytes_) / totalBytes_;
	}

private:
	Socket& socket_;          // 引用 Socket 对象
	uint64_t totalBytes_;     // 文件总大小
	uint64_t transferredBytes_; // 已传输字节数

	// 打印进度条
	void printProgress() const {
		double progress = getProgress();
		int barWidth = 50;

		std::cout << "[";
		int pos = static_cast<int>(barWidth * progress);
		for (int i = 0; i < barWidth; ++i) {
			if (i < pos) std::cout << "#";
			else if (i == pos) std::cout << ">";
			else std::cout << "-";
		}
		std::cout << "] " << std::setw(3) << static_cast<int>(progress * 100.0) << " %\r";
		std::cout.flush();
	}

	// 获取文件大小
	uint64_t getFileSize(const std::string& filePath) const {
		std::ifstream file(filePath, std::ios::binary | std::ios::ate);
		if (!file.is_open()) return 0;
		return file.tellg();
	}

	// 发送分片
	bool sendChunk(const char* data, std::streamsize size) {
		int retries = 0;
		while (retries < MAX_RETRIES) {
			if (socket_.send(std::string(data, size))) {
				return true;
			}
			retries++;
			printf("Retrying to send chunk... (%d/%d)\n", retries, MAX_RETRIES);
		}
		return false;
	}

	// 接收分片
	std::streamsize receiveChunk(char* buffer, std::streamsize size) {
		std::string data;
		if (socket_.recv(data)) {
			if (data.empty()) {
				return 0; // 接收到空字符串，表示传输完成
			}
			std::memcpy(buffer, data.data(), data.size());
			return data.size();
		}
		return -1; // 接收失败
	}
};

#endif // FILETRANSFER_HPP