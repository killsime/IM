#include "MySQLClient.hpp"

int main()
{
    try
    {
        // 创建工具类对象
        MySQLClient db("192.168.88.222", "root", "root", "demo_db");

        // 执行查询并打印结果
        auto result = db.query("SELECT * FROM user");
        MySQLClient::printResult(result);

        std::cout << "用户名字列表：" << std::endl;
        for (const auto &row : result)
        {
            // 检查是否存在 "name" 字段
            if (row.find("name") != row.end())
                std::cout << row.at("name") << " ";
            else
                std::cerr << "警告：查询结果中未找到 'name' 字段" << std::endl;
        }

        // 执行更新
        int affected_rows = db.execute("UPDATE user SET age = 30 WHERE id = 1");
        std::cout << "\n受影响的行数: " << affected_rows << std::endl;

        // 执行事务
        db.executeTransaction([&db]()
                              {
            db.execute("INSERT INTO user (name, age, email) VALUES ('赵六', 26, 'zhaoliu@example.com')");
            db.execute("UPDATE user SET age = 28 WHERE id = 2"); });
    }
    catch (const std::exception &e)
    {
        std::cerr << "错误: " << e.what() << std::endl;
    }

    return 0;
}