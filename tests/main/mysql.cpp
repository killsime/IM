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

        // 查询用户 ID 为 1 的订单,联表查询
        int user_id = 1;
        std::string query = "SELECT o.id AS order_id, o.order_date, o.status, o.total_amount "
                            "FROM `order` o "
                            "JOIN user u ON o.user_id = u.id "
                            "WHERE u.id = " +
                            std::to_string(user_id);

        result = db.query(query);

        // 打印订单信息
        std::cout << "\n用户 ID 为 " << user_id << " 的订单信息：" << std::endl;
        MySQLClient::printResult(result);

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