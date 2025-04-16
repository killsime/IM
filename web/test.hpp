#pragma once
#include "httplib.h"
#include "json.hpp"
#include <atomic>
#include <string>
#include <map>

using json = nlohmann::json;

class TestAPI
{
private:
    static std::atomic<bool> &get_running_flag()
    {
        static std::atomic<bool> flag(true);
        return flag;
    }

public:
    // 启动测试接口服务
    static void run()
    {
        httplib::Server svr;
        auto &keep_running = get_running_flag();
        keep_running = true;

        // 添加关闭接口
        svr.Get("/shutdown", [&](const httplib::Request &, httplib::Response &res)
                {
            keep_running = false;
            res.set_content("Server shutting down...", "text/plain"); });

        // 示例1：带查询参数的 GET 接口
        svr.Get("/api/user", [](const httplib::Request &req, httplib::Response &res)
                {
            auto name = req.get_param_value("name");
            auto age = req.get_param_value("age");

            json response = {
                {"status", "success"},
                {"data", {
                    {"name", name.empty() ? "Anonymous" : name},
                    {"age", age.empty() ? 0 : std::stoi(age)},
                    {"timestamp", time(nullptr)}
                }},
                {"metadata", {
                    {"method", "GET"},
                    {"path", "/api/user"}
                }}
            };
            res.set_content(response.dump(), "application/json"); });

        // 示例2：带路径参数的 GET 接口
        svr.Get(R"(/api/books/(\d+))", [](const httplib::Request &req, httplib::Response &res)
                {
            auto book_id = req.matches[1];
            std::map<std::string, json> books = {
                {"1", {{"title", "C++ Primer"}, {"price", 59.9}}},
                {"2", {{"title", "Design Patterns"}, {"price", 39.9}}}
            };

            if (books.count(book_id)) {
                res.set_content(json{{"book", books[book_id]}}.dump(), "application/json");
            } else {
                res.status = 404;
                res.set_content(json{{"error", "Book not found"}}.dump(), "application/json");
            } });

        std::cout << "Server running at http://localhost:8080\n";

        // 修改后的监听调用（不使用lambda）
        while (keep_running)
        {
            if (!svr.listen("localhost", 8080))
            {
                std::cerr << "Server listen error\n";
                break;
            }
        }
    }
};