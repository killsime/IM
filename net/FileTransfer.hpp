#ifndef FILETRANSFER_HPP
#define FILETRANSFER_HPP

#include "Socket.hpp"
#include "FileUtils.hpp"
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <iostream>
#include <iomanip>
#include <cstring>

#define CHUNK_SIZE 1024 * 1024     // 每个分片的大小（1MB）
#define DEFAULT_REPO_PATH "./repo" // 默认文件存储路径

template <typename T>
T MIN(T a, T b)
{
    return a < b ? a : b;
}

class FileTransfer
{
public:
    FileTransfer(Socket &socket) : socket_(socket), totalBytes_(0), transferredBytes_(0)
    {
        setRepoPath(DEFAULT_REPO_PATH);
        socket_.optimizeForLargeFileTransfer();
    }

    void setRepoPath(const std::string &path)
    {
        repoPath_ = path;
        if (!FileUtils::fileExists(repoPath_))
        {
            FileUtils::createDirectory(repoPath_);
        }
    }

    std::string getRepoPath() const
    {
        return repoPath_;
    }

    bool sendFile(const std::string &fileName)
    {
        std::string filePath = FileUtils::joinPath({repoPath_, fileName});
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            std::cerr << "Failed to open file: " << filePath << std::endl;
            return false;
        }

        totalBytes_ = file.tellg();
        file.seekg(0, std::ios::beg);

        // 等待，确保服务器准备好接收
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        if (!sendFileSize(totalBytes_))
        {
            std::cerr << "Failed to send file size." << std::endl;
            file.close();
            return false;
        }
        // 等待, 防止数据包与文件大小沾包
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // 发送文件内容
        std::vector<char> buffer(CHUNK_SIZE);
        transferredBytes_ = 0;
        size_t sequenceNumber = 0; // 序号计数器

        while (transferredBytes_ < totalBytes_)
        {
            file.read(buffer.data(), CHUNK_SIZE);
            std::streamsize bytesRead = file.gcount();

            size_t sent = 0;
            while (sent < static_cast<size_t>(bytesRead))
            {
                std::vector<char> chunk(buffer.data() + sent, buffer.data() + bytesRead);
                size_t result = socket_.send(chunk);
                if (result == 0)
                {
                    std::cerr << "Failed to send file chunk." << std::endl;
                    file.close();
                    return false;
                }
                sent += result;
                transferredBytes_ += result;

                // 每 50 次操作刷新一次进度
                if (++sequenceNumber % 50 == 0)
                {
                    double progress = static_cast<double>(transferredBytes_) / totalBytes_ * 100;
                    std::cout << "\rSending: " << transferredBytes_ << " / " << totalBytes_
                              << " bytes (" << std::fixed << std::setprecision(2) << progress << "%)";
                    std::cout.flush();
                }
            }
        }

        // 最后刷新一次进度，确保显示 100%
        std::cout << "\rSending: " << transferredBytes_ << " / " << totalBytes_
                  << " bytes (100.00%)" << std::endl;

        file.close();
        std::cout << "File sent successfully." << std::endl;
        return true;
    }

    bool receiveFile(const std::string &fileName)
    {
        std::string filePath = FileUtils::joinPath({repoPath_, fileName});
        std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
        {
            std::cerr << "Failed to create file: " << filePath << std::endl;
            return false;
        }

        // 接收文件大小
        if (!receiveFileSize(totalBytes_))
        {
            std::cerr << "Failed to receive file size." << std::endl;
            file.close();
            return false;
        }

        // 接收文件内容
        transferredBytes_ = 0;
        size_t sequenceNumber = 0; // 序号计数器

        size_t received = 0;
        while (received < totalBytes_)
        {
            std::vector<char> data;
            size_t result = socket_.recv(data);
            if (result == 0)
            {
                std::cerr << "Failed to receive file chunk." << std::endl;
                file.close();
                return false;
            }

            size_t readSize = MIN(result, totalBytes_ - received);
            file.write(data.data(), readSize);
            received += readSize;
            transferredBytes_ += readSize;

            // 每 50 次操作刷新一次进度
            if (++sequenceNumber % 50 == 0)
            {
                double progress = static_cast<double>(transferredBytes_) / totalBytes_ * 100;
                std::cout << "\rReceiving: " << transferredBytes_ << " / " << totalBytes_
                          << " bytes (" << std::fixed << std::setprecision(2) << progress << "%)";
                std::cout.flush();
            }
        }

        // 最后刷新一次进度，确保显示 100%
        std::cout << "\rReceiving: " << transferredBytes_ << " / " << totalBytes_
                  << " bytes (100.00%)" << std::endl;

        file.close();
        std::cout << "File received successfully." << std::endl;
        return true;
    }

    uint64_t getTransferredBytes() const
    {
        return transferredBytes_;
    }

    uint64_t getTotalBytes() const
    {
        return totalBytes_;
    }

private:
    Socket &socket_;
    uint64_t totalBytes_;
    uint64_t transferredBytes_;
    std::string repoPath_;

    bool sendFileSize(uint64_t fileSize)
    {
        std::vector<char> sizeData(sizeof(fileSize));
        std::memcpy(sizeData.data(), &fileSize, sizeof(fileSize));
        size_t result = socket_.send(sizeData);

        std::cout << "Sent file size: " << fileSize << " bytes. " << std::endl;

        return result > 0;
    }

    bool receiveFileSize(uint64_t &fileSize)
    {
        std::vector<char> sizeData(sizeof(fileSize));
        size_t result = socket_.recv(sizeData);

        if (result > 0)
        {
            std::memcpy(&fileSize, sizeData.data(), sizeof(fileSize));
            std::cout << "Received file size: " << fileSize << " bytes. " << std::endl;
            return true;
        }
        return false;
    }
};

#endif // FILETRANSFER_HPP