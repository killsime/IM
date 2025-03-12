#ifndef FILETRANSFER_HPP
#define FILETRANSFER_HPP

#include "Socket.hpp" // 引用 Socket 类
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <cstring>


// 定义常量
#define CHUNK_SIZE 1024 * 1024        // 每个分片的大小（1MB）
#define MAX_RETRIES 3               // 最大重试次数
#define END_MARKER "FILE_END_MARKER" // 文件传输结束标志

class FileTransfer {
public:
    // 构造函数
    FileTransfer(Socket& socket) : socket_(socket), totalBytes_(0), transferredBytes_(0) {
        socket_.setBufferSize(CHUNK_SIZE);
    }

    bool sendFile(const std::string& filePath, uint64_t offset = 0) {
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filePath << std::endl;
            return false;
        }

        // 获取文件大小
        totalBytes_ = file.tellg();
        file.seekg(offset, std::ios::beg);

        // 读取文件并分片发送
        std::vector<char> buffer(CHUNK_SIZE);
        while (!file.eof()) {
            file.read(buffer.data(), CHUNK_SIZE);
            std::streamsize bytesRead = file.gcount();

            // 发送分片
            if (!sendChunk(buffer.data(), bytesRead)) {
                std::cerr << "Failed to send file chunk." << std::endl;
                file.close();
                return false;
            }

            // 更新已传输字节数
            transferredBytes_ += bytesRead;
        }

        // 发送结束标志
        if (!sendChunk(END_MARKER, strlen(END_MARKER))) {
            std::cerr << "Failed to send end marker." << std::endl;
            file.close();
            return false;
        }

        file.close();
        std::cout << "File sent successfully." << std::endl;
        return true;
    }

    bool receiveFile(const std::string& filePath, uint64_t offset = 0) {
        std::ofstream file(filePath, std::ios::binary | std::ios::trunc); // 使用 trunc 模式
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filePath << std::endl;
            return false;
        }
        
        // 获取文件大小（如果已知）
        // totalBytes_ = getFileSize(filePath) + offset;

        // 接收分片并写入文件
        std::vector<char> buffer(CHUNK_SIZE);
        while (true) {
            std::streamsize bytesReceived = receiveChunk(buffer.data(), CHUNK_SIZE);
            if (bytesReceived == 0) {
                // 传输完成
                break;
            }
            else if (bytesReceived == -1) {
                std::cerr << "Failed to receive file chunk." << std::endl;
                file.close();
                return false;
            }

            // 检查是否为结束标志
            if (std::string(buffer.data(), bytesReceived) == END_MARKER) {
                std::cout << "Received end marker." << std::endl;
                break;
            }

            file.write(buffer.data(), bytesReceived);

            // 更新已传输字节数
            transferredBytes_ += bytesReceived;
        }

        file.close();
        std::cout << "File received successfully." << std::endl;
        return true;
    }

    // 获取已传输字节数
    uint64_t getTransferredBytes() const {
        return transferredBytes_;
    }

    // 获取文件总大小
    uint64_t getTotalBytes() const {
        return totalBytes_;
    }

private:
    Socket& socket_;          // 引用 Socket 对象
    uint64_t totalBytes_;     // 文件总大小
    uint64_t transferredBytes_; // 已传输字节数

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
            std::cerr << "Retrying to send chunk... (" << retries << "/" << MAX_RETRIES << ")" << std::endl;
        }
        return false;
    }

    // 接收分片
	std::streamsize receiveChunk(char* buffer, std::streamsize size) {
		std::string data;
		if (socket_.recv(data)) { // 尝试接收数据
			if (data == END_MARKER) {
				return 0; // 接收到结束标志，表示传输完成
			}
			std::memcpy(buffer, data.data(), data.size());
			return data.size(); // 返回接收到的数据大小
		}
		return -1; // 接收失败
	}
};

// 打印进度条的函数
void printProgress(const FileTransfer& ft) {
    auto startTime = std::chrono::steady_clock::now();
    while (ft.getTransferredBytes() < ft.getTotalBytes()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        double progress = static_cast<double>(ft.getTransferredBytes()) / ft.getTotalBytes();
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
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    std::cout << "\nTransfer completed in " << duration << " ms. Transferred " << ft.getTransferredBytes() << " bytes." << std::endl;
}

#endif // FILETRANSFER_HPP