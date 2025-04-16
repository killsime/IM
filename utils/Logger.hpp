#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdarg>
#include <ctime>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <atomic>
#include <memory>
#include <thread>
#include <mutex>

// 常量宏定义
#define LOGGER_SOCKET_PATH "/tmp/logger.sock"
#define LOGGER_BUFFER_SIZE 1024
#define LOGGER_MESSAGE_SIZE 1024
#define LOGGER_FULL_MSG_SIZE 2048
#define LOGGER_TIMESTAMP_FORMAT "%Y-%m-%d %H:%M:%S"
#define LOGGER_TIMESTAMP_SIZE 32

class Logger
{
public:
    enum class Level
    {
        INFO,
        WARNING,
        ERROR
    };

    // 获取单例实例
    static Logger &instance()
    {
        static Logger logger;
        return logger;
    }

    // 初始化日志系统
    bool initialize(bool start_server = true, const std::string &path = LOGGER_SOCKET_PATH)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (initialized_)
        {
            return true;
        }

        socket_path_ = path;

        if (start_server)
        {
            start_server_ = true;
            if (!start_log_server())
            {
                return false;
            }
            // 给服务器一点启动时间
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (!init_client())
        {
            return false;
        }

        initialized_ = true;
        return true;
    }

    // 日志记录接口
    void log(Level level, const char *file, int line, const char *format, ...)
    {
        if (!initialized_ && !initialize())
        {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        if (client_socket_ == -1 && !init_client())
        {
            return;
        }

        va_list args;
        va_start(args, format);
        log_internal(level, file, line, format, args);
        va_end(args);
    }

private:
    Logger() = default;
    ~Logger()
    {
        if (client_socket_ != -1)
        {
            close(client_socket_);
        }
        if (server_thread_.joinable())
        {
            server_thread_.join();
        }
    }

    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    bool init_client()
    {
        client_socket_ = socket(AF_UNIX, SOCK_STREAM, 0);
        if (client_socket_ == -1)
        {
            return false;
        }

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

        if (connect(client_socket_, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        {
            close(client_socket_);
            client_socket_ = -1;
            return false;
        }

        // 设置非阻塞
        int flags = fcntl(client_socket_, F_GETFL, 0);
        fcntl(client_socket_, F_SETFL, flags | O_NONBLOCK);

        return true;
    }

    bool start_log_server()
    {
        try
        {
            server_thread_ = std::thread([this]()
                                         {
                int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
                if (server_fd == -1) {
                    return;
                }

                struct sockaddr_un addr;
                memset(&addr, 0, sizeof(addr));
                addr.sun_family = AF_UNIX;
                strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

                unlink(socket_path_.c_str());
                if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
                    close(server_fd);
                    return;
                }

                if (listen(server_fd, 5) == -1) {
                    close(server_fd);
                    return;
                }

                std::ofstream logfile("application.log", std::ios::app);
                if (!logfile.is_open()) {
                    close(server_fd);
                    return;
                }

                while (true) {
                    int client_fd = accept(server_fd, nullptr, nullptr);
                    if (client_fd == -1) {
                        continue;
                    }

                    char buffer[LOGGER_BUFFER_SIZE];
                    ssize_t bytes_read;
                    std::string remaining_data;
                    
                    while ((bytes_read = read(client_fd, buffer, sizeof(buffer) - 1)) > 0) {
                        buffer[bytes_read] = '\0';
                        remaining_data += buffer;

                        size_t pos;
                        while ((pos = remaining_data.find('\n')) != std::string::npos) {
                            std::string message = remaining_data.substr(0, pos);
                            remaining_data.erase(0, pos + 1);

                            if (!message.empty()) {
                                auto now = std::chrono::system_clock::now();
                                std::time_t now_c = std::chrono::system_clock::to_time_t(now);
                                std::tm now_tm = *std::localtime(&now_c);
                                
                                char timestamp[LOGGER_TIMESTAMP_SIZE];
                                std::strftime(timestamp, sizeof(timestamp), LOGGER_TIMESTAMP_FORMAT, &now_tm);
                                
                                std::lock_guard<std::mutex> lock(server_mutex_);
                                logfile << "[" << timestamp << "] " << message << std::endl;
                                std::cout << "[" << timestamp << "] " << message << std::endl;
                            }
                        }
                    }
                    
                    close(client_fd);
                } });
        }
        catch (...)
        {
            return false;
        }

        return true;
    }

    void log_internal(Level level, const char *file, int line, const char *format, va_list args)
    {
        char message[LOGGER_MESSAGE_SIZE];
        char full_message[LOGGER_FULL_MSG_SIZE];

        const char *level_str = "";
        switch (level)
        {
        case Level::INFO:
            level_str = "INFO";
            break;
        case Level::WARNING:
            level_str = "WARN";
            break;
        case Level::ERROR:
            level_str = "ERROR";
            break;
        }

        vsnprintf(message, sizeof(message), format, args);
        snprintf(full_message, sizeof(full_message), "[%s] %s:%d %s\n",
                 level_str, file, line, message);

        if (client_socket_ != -1)
        {
            send(client_socket_, full_message, strlen(full_message), 0);
        }
    }

    std::string socket_path_ = LOGGER_SOCKET_PATH;
    int client_socket_ = -1;
    std::thread server_thread_;
    std::mutex mutex_;
    std::mutex server_mutex_;
    std::atomic<bool> initialized_{false};
    bool start_server_ = false;
};

// 便捷宏
#define LOG_INFO(format, ...) \
    Logger::instance().log(Logger::Level::INFO, __FILE__, __LINE__, format, ##__VA_ARGS__);

#define LOG_WARN(format, ...) \
    Logger::instance().log(Logger::Level::WARNING, __FILE__, __LINE__, format, ##__VA_ARGS__);

#define LOG_ERROR(format, ...) \
    Logger::instance().log(Logger::Level::ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__);

#endif // LOGGER_HPP