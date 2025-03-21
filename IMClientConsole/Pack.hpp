#ifndef PACK_HPP
#define PACK_HPP

#include <vector>
#include <cstdint>
#include <stdexcept>
#include <iomanip>
#include <iostream>

class Pack
{
private:
	uint16_t sHead;             // 包头 (0xFEFF)
	uint32_t nLength;           // 数据长度 (包括类型和校验和)
	uint16_t sType;             // 数据类型
	uint16_t sSum;              // 校验和
	std::vector<char> byteData; // 数据内容

public:
	// 封包构造函数
	Pack(uint16_t sType, const std::vector<char>& data)
	{
		this->sHead = 0xFEFF;                                   // 固定包头
		this->nLength = static_cast<uint32_t>(data.size() + 4); // 数据长度 = 数据体长度 + 类型(2字节) + 校验和(2字节)
		this->sType = sType;
		this->byteData = data;

		// 计算校验和
		this->sSum = 0;
		for (char c : data)
		{
			this->sSum += static_cast<uint8_t>(c);
		}
	}

	// 解包构造函数
	Pack(const std::vector<char>& byteStream)
	{
		if (byteStream.size() < 10)
		{ // 最小包大小: 包头(2) + 长度(4) + 类型(2) + 校验和(2)
			throw std::runtime_error("Invalid packet size");
		}

		// 校验包头
		if (static_cast<uint8_t>(byteStream[0]) != 0xFE || static_cast<uint8_t>(byteStream[1]) != 0xFF)
		{
			throw std::runtime_error("Invalid packet header");
		}

		// 解析长度
		this->nLength = (static_cast<uint8_t>(byteStream[2]) << 24) |
			(static_cast<uint8_t>(byteStream[3]) << 16) |
			(static_cast<uint8_t>(byteStream[4]) << 8) |
			static_cast<uint8_t>(byteStream[5]);
		if (nLength + 6 > byteStream.size())
		{ // 包不完整
			throw std::runtime_error("Incomplete packet");
		}

		// 解析类型
		this->sType = (static_cast<uint8_t>(byteStream[6]) << 8) | static_cast<uint8_t>(byteStream[7]);

		// 解析数据
		size_t dataSize = nLength - 4; // 数据长度 = 总长度 - 类型(2) - 校验和(2)
		if (dataSize > 0)
		{
			this->byteData.assign(byteStream.begin() + 8, byteStream.begin() + 8 + dataSize);
		}

		// 校验和验证
		this->sSum = 0;
		for (char c : this->byteData)
		{
			this->sSum += static_cast<uint8_t>(c);
		}

		uint16_t checksum = (static_cast<uint8_t>(byteStream[8 + dataSize]) << 8) |
			static_cast<uint8_t>(byteStream[9 + dataSize]);
		if (this->sSum != checksum)
		{
			throw std::runtime_error("Checksum error");
		}
	}

	// 获取类型
	uint16_t getType() const
	{
		return this->sType;
	}

	// 获取数据
	const std::vector<char>& getData() const
	{
		return this->byteData;
	}

	// 将封包对象转换为字节流
	std::vector<char> toByteStream() const
	{
		std::vector<char> byteStream;

		// 添加包头
		byteStream.push_back(static_cast<char>((sHead >> 8) & 0xFF));
		byteStream.push_back(static_cast<char>(sHead & 0xFF));

		// 添加长度
		byteStream.push_back(static_cast<char>((nLength >> 24) & 0xFF));
		byteStream.push_back(static_cast<char>((nLength >> 16) & 0xFF));
		byteStream.push_back(static_cast<char>((nLength >> 8) & 0xFF));
		byteStream.push_back(static_cast<char>(nLength & 0xFF));

		// 添加类型
		byteStream.push_back(static_cast<char>((sType >> 8) & 0xFF));
		byteStream.push_back(static_cast<char>(sType & 0xFF));

		// 添加数据
		byteStream.insert(byteStream.end(), byteData.begin(), byteData.end());

		// 添加校验和
		byteStream.push_back(static_cast<char>((sSum >> 8) & 0xFF));
		byteStream.push_back(static_cast<char>(sSum & 0xFF));

		return byteStream;
	}

	// 输出字节流
	void dump() const
	{
		std::vector<char> byteStream = toByteStream();
		for (char c : byteStream)
		{
			std::cout << std::hex << std::setw(2) << std::setfill('0') << (static_cast<uint8_t>(c) & 0xFF) << " ";
		}
		std::cout << std::endl;
	}
};

#endif // PACK_HPP