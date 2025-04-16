#include "test.hpp"
#include "gtest/gtest.h"
#include <thread>
#include <chrono>

class TestAPITest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        server_thread = std::thread([]
                                    { TestAPI::run(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    static void TearDownTestSuite()
    {
        httplib::Client cli("localhost", 8080);
        auto res = cli.Get("/shutdown");
        if (res)
        {
            std::cout << "Shutdown response: " << res->body << std::endl;
        }
        server_thread.join();
    }

    static std::thread server_thread;
};

std::thread TestAPITest::server_thread;

TEST_F(TestAPITest, GetUserAPI)
{
    httplib::Client cli("localhost", 8080);

    // 测试正常参数
    auto res1 = cli.Get("/api/user?name=Alice&age=25");
    ASSERT_TRUE(res1 != nullptr);
    EXPECT_EQ(res1->status, 200);

    auto json1 = nlohmann::json::parse(res1->body);
    EXPECT_EQ(json1["status"], "success");
    EXPECT_EQ(json1["data"]["name"], "Alice");
    EXPECT_EQ(json1["data"]["age"], 25);

    // 测试缺省参数
    auto res2 = cli.Get("/api/user");
    auto json2 = nlohmann::json::parse(res2->body);
    EXPECT_EQ(json2["data"]["name"], "Anonymous");
    EXPECT_EQ(json2["data"]["age"], 0);
}

TEST_F(TestAPITest, GetBookAPI)
{
    httplib::Client cli("localhost", 8080);

    // 测试存在的书籍
    auto res1 = cli.Get("/api/books/1");
    ASSERT_TRUE(res1 != nullptr);
    EXPECT_EQ(res1->status, 200);

    auto json1 = nlohmann::json::parse(res1->body);
    EXPECT_EQ(json1["book"]["title"], "C++ Primer");
    EXPECT_DOUBLE_EQ(json1["book"]["price"], 59.9);

    // 测试不存在的书籍
    auto res2 = cli.Get("/api/books/999");
    EXPECT_EQ(res2->status, 404);
    auto json2 = nlohmann::json::parse(res2->body);
    EXPECT_EQ(json2["error"], "Book not found");
}