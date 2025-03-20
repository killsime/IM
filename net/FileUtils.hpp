#ifndef FILE_UTILS_HPP
#define FILE_UTILS_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#endif

class FileUtils
{
public:
    // 拼接路径
    static std::string joinPath(const std::vector<std::string> &parts)
    {
        if (parts.empty())
            return "";

        std::string result = parts[0];
        for (size_t i = 1; i < parts.size(); ++i)
        {
            if (!result.empty() && result.back() != '/')
                result += '/';
            result += parts[i];
        }
        return result;
    }

    // 规范化路径
    static std::string normalizePath(const std::string &path)
    {
#ifdef _WIN32
        wchar_t buffer[MAX_PATH];
        std::wstring wpath = stringToWstring(path);
        if (PathCanonicalizeW(buffer, wpath.c_str()))
            return wstringToString(buffer);
        throw std::runtime_error("Failed to normalize path: " + path);
#else
        char buffer[PATH_MAX];
        if (realpath(path.c_str(), buffer) != nullptr)
            return std::string(buffer);
        throw std::runtime_error("Failed to normalize path: " + path);
#endif
    }

    // 获取文件名（不含路径）
    static std::string getFileName(const std::string &path)
    {
#ifdef _WIN32
        std::wstring wpath = stringToWstring(path);
        return wstringToString(PathFindFileNameW(wpath.c_str()));
#else
        size_t pos = path.find_last_of('/');
        if (pos == std::string::npos)
            return path;
        return path.substr(pos + 1);
#endif
    }

    // 获取文件扩展名
    static std::string getFileExtension(const std::string &path)
    {
#ifdef _WIN32
        std::wstring wpath = stringToWstring(path);
        return wstringToString(PathFindExtensionW(wpath.c_str()));
#else
        size_t pos = path.find_last_of('.');
        if (pos == std::string::npos)
            return "";
        return path.substr(pos);
#endif
    }

    // 获取文件大小（字节）
    static uint64_t getFileSize(const std::string &path)
    {
#ifdef _WIN32
        WIN32_FILE_ATTRIBUTE_DATA fileInfo;
        std::wstring wpath = stringToWstring(path);
        if (!GetFileAttributesExW(wpath.c_str(), GetFileExInfoStandard, &fileInfo))
            throw std::runtime_error("Failed to get file size: " + path);
        return (static_cast<uint64_t>(fileInfo.nFileSizeHigh) << 32) | fileInfo.nFileSizeLow;
#else
        struct stat statBuf;
        if (stat(path.c_str(), &statBuf) != 0)
            throw std::runtime_error("Failed to get file size: " + path);
        return static_cast<uint64_t>(statBuf.st_size);
#endif
    }

    // 检查文件是否存在
    static bool fileExists(const std::string &path)
    {
#ifdef _WIN32
        std::wstring wpath = stringToWstring(path);
        return PathFileExistsW(wpath.c_str()) == TRUE;
#else
        struct stat statBuf;
        return stat(path.c_str(), &statBuf) == 0;
#endif
    }

    // 获取文件所在目录
    static std::string getFileDirectory(const std::string &path)
    {
#ifdef _WIN32
        std::wstring wpath = stringToWstring(path);
        wchar_t buffer[MAX_PATH];
        if (wcscpy_s(buffer, wpath.c_str()) == 0) // 使用 wcscpy_s
            return wstringToString(buffer);
        return "";
#else
        size_t pos = path.find_last_of('/');
        if (pos == std::string::npos)
            return "";
        return path.substr(0, pos);
#endif
    }

    // 获取当前工作目录
    static std::string getCurrentWorkingDirectory()
    {
#ifdef _WIN32
        wchar_t buffer[MAX_PATH];
        if (_wgetcwd(buffer, MAX_PATH) != nullptr)
            return wstringToString(buffer);
        throw std::runtime_error("Failed to get current working directory");
#else
        char buffer[PATH_MAX];
        if (getcwd(buffer, PATH_MAX) != nullptr)
            return std::string(buffer);
        throw std::runtime_error("Failed to get current working directory");
#endif
    }

    // 创建目录（包括父目录）
    static bool createDirectory(const std::string &path)
    {
#ifdef _WIN32
        std::wstring wpath = stringToWstring(path);
        return CreateDirectoryW(wpath.c_str(), nullptr) == TRUE || GetLastError() == ERROR_ALREADY_EXISTS;
#else
        return mkdir(path.c_str(), 0777) == 0 || errno == EEXIST;
#endif
    }

    // 删除文件
    static bool deleteFile(const std::string &path)
    {
#ifdef _WIN32
        std::wstring wpath = stringToWstring(path);
        return DeleteFileW(wpath.c_str()) == TRUE;
#else
        return unlink(path.c_str()) == 0;
#endif
    }

    // 删除目录（递归删除）
    static bool deleteDirectory(const std::string &path)
    {
#ifdef _WIN32
        std::wstring wpath = stringToWstring(path);
        return RemoveDirectoryW(wpath.c_str()) == TRUE;
#else
        return rmdir(path.c_str()) == 0;
#endif
    }
#ifdef _WIN32
private:
    // 将 std::string 转换为 std::wstring
    static std::wstring stringToWstring(const std::string &str)
    {
        if (str.empty())
            return L"";
        int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        std::wstring wstr(size, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);
        return wstr;
    }

    // 将 std::wstring 转换为 std::string
    static std::string wstringToString(const std::wstring &wstr)
    {
        if (wstr.empty())
            return "";
        int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string str(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size, nullptr, nullptr);
        return str;
    }
#endif
};

#endif // FILE_UTILS_HPP