#include <iostream>
#include <mysql/mysql.h>
#include <vector>
#include <map>
#include <stdexcept>
#include <string>
#include <functional>

class MySQLClient
{
public:
    // 构造函数
    MySQLClient(const std::string &host, const std::string &user, const std::string &password, const std::string &database)
        : host_(host), user_(user), password_(password), database_(database), conn_(nullptr)
    {
        connect();
    }

    // 析构函数
    ~MySQLClient()
    {
        disconnect();
    }

    // 连接数据库
    void connect()
    {
        conn_ = mysql_init(nullptr);
        if (!conn_)
        {
            throw std::runtime_error("MySQL 初始化失败");
        }

        if (!mysql_real_connect(conn_, host_.c_str(), user_.c_str(), password_.c_str(), database_.c_str(), 0, nullptr, 0))
        {
            throw std::runtime_error("连接数据库失败: " + std::string(mysql_error(conn_)));
        }

        // 设置字符集为 utf8mb4
        if (mysql_set_character_set(conn_, "utf8mb4"))
        {
            throw std::runtime_error("设置字符集失败: " + std::string(mysql_error(conn_)));
        }
    }

    // 断开连接
    void disconnect()
    {
        if (conn_)
        {
            mysql_close(conn_);
            conn_ = nullptr;
        }
    }

    // 执行查询
    std::vector<std::map<std::string, std::string>> query(const std::string &sql)
    {
        if (mysql_query(conn_, sql.c_str()))
        {
            throw std::runtime_error("查询失败: " + std::string(mysql_error(conn_)));
        }

        MYSQL_RES *res = mysql_store_result(conn_);
        if (!res)
        {
            throw std::runtime_error("获取结果集失败: " + std::string(mysql_error(conn_)));
        }

        std::vector<std::map<std::string, std::string>> result;
        MYSQL_ROW row;
        MYSQL_FIELD *fields = mysql_fetch_fields(res);
        int num_fields = mysql_num_fields(res);

        while ((row = mysql_fetch_row(res)))
        {
            std::map<std::string, std::string> row_map;
            for (int i = 0; i < num_fields; i++)
            {
                row_map[fields[i].name] = row[i] ? row[i] : "NULL";
            }
            result.push_back(row_map);
        }

        mysql_free_result(res);
        return result;
    }

    // 执行更新
    int execute(const std::string &sql)
    {
        if (mysql_query(conn_, sql.c_str()))
        {
            throw std::runtime_error("执行失败: " + std::string(mysql_error(conn_)));
        }
        return mysql_affected_rows(conn_);
    }

    // 执行事务
    void executeTransaction(const std::function<void()> &func)
    {
        try
        {
            beginTransaction();
            func();
            commit();
        }
        catch (const std::exception &e)
        {
            rollback();
            throw std::runtime_error("事务回滚: " + std::string(e.what()));
        }
    }

    // 静态函数：打印查询结果
    static void printResult(const std::vector<std::map<std::string, std::string>> &result)
    {
        if (result.empty())
        {
            std::cout << "查询结果为空" << std::endl;
            return;
        }

        // 打印表头
        for (const auto &field : result[0])
        {
            std::cout << field.first << "\t";
        }
        std::cout << std::endl;

        // 打印分隔线
        for (const auto &field : result[0])
        {
            std::cout << "--------\t";
        }
        std::cout << std::endl;

        // 打印数据
        for (const auto &row : result)
        {
            for (const auto &field : row)
            {
                std::cout << field.second << "\t";
            }
            std::cout << std::endl;
        }
    }

private:
    std::string host_;     // 主机名
    std::string user_;     // 用户名
    std::string password_; // 密码
    std::string database_; // 数据库名
    MYSQL *conn_;          // MySQL 连接对象

    // 开启事务
    void beginTransaction()
    {
        if (mysql_query(conn_, "START TRANSACTION"))
        {
            throw std::runtime_error("开启事务失败: " + std::string(mysql_error(conn_)));
        }
    }

    // 提交事务
    void commit()
    {
        if (mysql_query(conn_, "COMMIT"))
        {
            throw std::runtime_error("提交事务失败: " + std::string(mysql_error(conn_)));
        }
    }

    // 回滚事务
    void rollback()
    {
        if (mysql_query(conn_, "ROLLBACK"))
        {
            throw std::runtime_error("回滚事务失败: " + std::string(mysql_error(conn_)));
        }
    }
};