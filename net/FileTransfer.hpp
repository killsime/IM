#ifndef FILETRANSFER_HPP
#define FILETRANSFER_HPP

#include "Socket.hpp"    // 引用 Socket 类
#include "FileUtils.hpp" // 引用跨平台文件操作库
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <iostream>
#include <iomanip>
#include <cstring>

// 定义常量
#define CHUNK_SIZE 1024 * 1024     // 每个分片的大小（1MB）
#define MAX_RETRIES 3              // 最大重试次数
#define DEFAULT_REPO_PATH "./repo" // 默认文件存储路径

class FileTransfer
{
public:
    FileTransfer(Socket &socket) : socket_(socket), totalBytes_(0), transferredBytes_(0)
    {
        setRepoPath(DEFAULT_REPO_PATH);
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
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // 发送文件大小
        if (!sendFileSize(totalBytes_))
        {
            std::cerr << "Failed to send file size." << std::endl;
            file.close();
            return false;
        }

        std::vector<char> buffer(CHUNK_SIZE);
        transferredBytes_ = 0;

        while (transferredBytes_ < totalBytes_)
        {
            file.read(buffer.data(), CHUNK_SIZE);
            std::streamsize bytesRead = file.gcount();

            if (!sendChunk(buffer.data(), bytesRead))
            {
                std::cerr << "Failed to send file chunk." << std::endl;
                file.close();
                return false;
            }
            transferredBytes_ += bytesRead;
        }

        file.close();
        socket_.close();
        std::cout << "File sent successfully." << std::endl;
        return true;
    }

    bool receiveFile(const std::string &fileName)
    {
        std::string filePath = FileUtils::joinPath({repoPath_, fileName});
        std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
        {
            std::cerr << "Failed to open file: " << filePath << std::endl;
            return false;
        }

        // 接收文件大小
        if (!receiveFileSize(totalBytes_))
        {
            std::cerr << "Failed to receive file size." << std::endl;
            file.close();
            return false;
        }

        transferredBytes_ = 0;
        std::vector<char> buffer(CHUNK_SIZE);

        while (transferredBytes_ < totalBytes_)
        {
            uint64_t bytesToRead = std::min<uint64_t>(CHUNK_SIZE, totalBytes_ - transferredBytes_);

            if (!receiveChunk(buffer.data(), bytesToRead))
            {
                std::cerr << "Failed to receive file chunk. Progress: " << transferredBytes_ << " / " << totalBytes_ << std::endl;
                file.close();
                return false;
            }
            file.write(buffer.data(), bytesToRead);
            transferredBytes_ += bytesToRead;
        }

        file.close();
        socket_.close();
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
        return socket_.send(sizeData);
    }

    bool receiveFileSize(uint64_t &fileSize)
    {
        std::vector<char> sizeData(sizeof(fileSize));
        if (!socket_.recv(sizeData))
        {
            return false;
        }
        std::memcpy(&fileSize, sizeData.data(), sizeof(fileSize));
        std::cout << "filesize : " << fileSize << std::endl;
        return true;
    }

    bool sendChunk(const char *data, std::streamsize size)
    {
        int retries = 0;
        std::size_t sent = 0;

        while (sent < static_cast<std::size_t>(size) && retries < MAX_RETRIES)
        {
            std::size_t remaining = size - sent;
            std::vector<char> chunk(data + sent, data + sent + remaining);
            if (socket_.send(chunk))
            {
                sent += chunk.size();
            }
            else
            {
                retries++;
                std::cerr << "Retrying to send chunk... (" << retries << "/" << MAX_RETRIES << ")" << std::endl;
            }
        }
        return sent == static_cast<std::size_t>(size);
    }

    bool receiveChunk(char *buffer, std::streamsize size)
    {
        int retries = 0;
        std::size_t received = 0;

        while (received < static_cast<std::size_t>(size) && retries < MAX_RETRIES)
        {
            std::vector<char> data;
            if (!socket_.recv(data) || data.empty())
            {
                retries++;
                std::cerr << "Retrying to receive chunk... (" << retries << "/" << MAX_RETRIES << ")" << std::endl;
                continue;
            }

            std::size_t chunkSize = std::min<std::size_t>(data.size(), (size - received));
            std::memcpy(buffer + received, data.data(), chunkSize);
            received += chunkSize;

            retries = 0;
        }

        return received == static_cast<std::size_t>(size);
    }
};

#endif // FILETRANSFER_HPP