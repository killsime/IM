#ifndef STREAMTRANSFER_HPP
#define STREAMTRANSFER_HPP

#include "Socket.hpp"
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>

#define MAX_RETRIES 3
#define ACK_SIZE sizeof(uint32_t)
#define READY_SIGNAL 0xFFFFFFFF
#define CHUNK_SIZE 8 * 1024 // 8KB

class StreamTransfer
{
public:
	// 发送方构造函数（主动发送文件大小）
	StreamTransfer(Socket& socket, uint64_t fileSize) : socket_(socket)
	{
		if (!confirmConnection() || !sendFileSize(fileSize))
			throw std::runtime_error("StreamTransfer init failed");
	}

	// 接收方构造函数（主动获取文件大小）
	StreamTransfer(Socket& socket) : socket_(socket)
	{
		if (!confirmConnection() || !receiveFileSize(fileSize_))
			throw std::runtime_error("StreamTransfer init failed");
	}

	// 发送封包（供FileTransfer调用）
	bool sendPacket(const std::vector<char>& data, uint32_t seq)
	{
		for (int retry = 0; retry < MAX_RETRIES; ++retry)
		{
			// 封装包头：序号(4B) + 数据
			std::vector<char> packet(sizeof(uint32_t) + data.size());
			uint32_t netSeq = htonl(seq);
			std::memcpy(packet.data(), &netSeq, sizeof(netSeq));
			std::memcpy(packet.data() + 4, data.data(), data.size());

			if (socket_.send(packet) != packet.size())
				continue;

			// 等待ACK
			if (waitForAck(seq, 500))
			{
				std::cout << "[Stream] Sent packet #" << seq << " (" << data.size() << " bytes)\n";
				return true;
			}
		}
		return false;
	}

	// 接收封包（供FileTransfer调用）
	bool receivePacket(std::vector<char>& data, uint32_t& seq)
	{
		// 接收包头
		std::vector<char> header(4);
		if (socket_.recv(header) != 4)
			return false;

		uint32_t netSeq;
		std::memcpy(&netSeq, header.data(), 4);
		seq = ntohl(netSeq);

		// 接收数据
		data.resize(CHUNK_SIZE);
		size_t received = socket_.recv(data);
		data.resize(received);

		// 发送ACK
		sendAck(seq);
		std::cout << "[Stream] Received packet #" << seq << " (" << received << " bytes)\n";
		return true;
	}

	// 接收方获取文件大小
	uint64_t getFileSize() const { return fileSize_; }

private:
	Socket& socket_;
	uint64_t fileSize_ = 0;

	bool confirmConnection()
	{
		// 发送/接收准备信号（类似三次握手）
		uint32_t netReady = htonl(READY_SIGNAL);
		return socket_.send({ reinterpret_cast<char*>(&netReady),
							 reinterpret_cast<char*>(&netReady) + 4 }) == 4 &&
			waitForReadySignal();
	}

	bool waitForReadySignal()
	{
		std::vector<char> buf(4);
		return socket_.recv(buf) == 4 &&
			ntohl(*reinterpret_cast<uint32_t*>(buf.data())) == READY_SIGNAL;
	}

	bool sendFileSize(uint64_t size)
	{
		std::vector<char> buf(8);
		std::memcpy(buf.data(), &size, 8);
		return socket_.send(buf) == 8;
	}

	bool receiveFileSize(uint64_t& size)
	{
		std::vector<char> buf(8);
		if (socket_.recv(buf) != 8)
			return false;
		std::memcpy(&size, buf.data(), 8);
		return true;
	}

	bool sendAck(uint32_t seq)
	{
		uint32_t netSeq = htonl(seq);
		return socket_.send({ reinterpret_cast<char*>(&netSeq),
							 reinterpret_cast<char*>(&netSeq) + 4 }) == 4;
	}

	bool waitForAck(uint32_t expectedSeq, int timeoutMs)
	{
		auto start = std::chrono::steady_clock::now();
		while (true)
		{
			std::vector<char> ack(4);
			if (socket_.recv(ack) == 4)
			{
				return ntohl(*reinterpret_cast<uint32_t*>(ack.data())) == expectedSeq;
			}

			if (std::chrono::steady_clock::now() - start >
				std::chrono::milliseconds(timeoutMs))
				break;

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		return false;
	}
};

#endif // STREAMTRANSFER_HPP