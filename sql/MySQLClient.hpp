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
    // Constructor
    MySQLClient(const std::string &host, const std::string &user, const std::string &password, const std::string &database)
        : host_(host), user_(user), password_(password), database_(database), conn_(nullptr)
    {
        connect();
    }

    // Destructor
    ~MySQLClient()
    {
        disconnect();
    }

    // Connect to the database
    void connect()
    {
        conn_ = mysql_init(nullptr);
        if (!conn_)
        {
            throw std::runtime_error("MySQL initialization failed");
        }

        if (!mysql_real_connect(conn_, host_.c_str(), user_.c_str(), password_.c_str(), database_.c_str(), 0, nullptr, 0))
        {
            throw std::runtime_error("Failed to connect to database: " + std::string(mysql_error(conn_)));
        }

        // Set character set to utf8mb4
        if (mysql_set_character_set(conn_, "utf8mb4"))
        {
            throw std::runtime_error("Failed to set character set: " + std::string(mysql_error(conn_)));
        }
    }

    // Disconnect from the database
    void disconnect()
    {
        if (conn_)
        {
            mysql_close(conn_);
            conn_ = nullptr;
        }
    }

    // Execute a query
    std::vector<std::map<std::string, std::string>> query(const std::string &sql)
    {
        if (mysql_query(conn_, sql.c_str()))
        {
            throw std::runtime_error("Query failed: " + std::string(mysql_error(conn_)));
        }

        MYSQL_RES *res = mysql_store_result(conn_);
        if (!res)
        {
            throw std::runtime_error("Failed to retrieve result set: " + std::string(mysql_error(conn_)));
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

    // Execute an update (INSERT, UPDATE, DELETE)
    int execute(const std::string &sql)
    {
        if (mysql_query(conn_, sql.c_str()))
        {
            throw std::runtime_error("Execution failed: " + std::string(mysql_error(conn_)));
        }
        return mysql_affected_rows(conn_);
    }

    // Execute a transaction
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
            throw std::runtime_error("Transaction rolled back: " + std::string(e.what()));
        }
    }

    // Static function: Print query results
    static void printResult(const std::vector<std::map<std::string, std::string>> &result)
    {
        if (result.empty())
        {
            std::cout << "Query result is empty" << std::endl;
            return;
        }

        // Print header
        for (const auto &field : result[0])
        {
            std::cout << field.first << "\t";
        }
        std::cout << std::endl;

        // Print data
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
    std::string host_;     // Hostname
    std::string user_;     // Username
    std::string password_; // Password
    std::string database_; // Database name
    MYSQL *conn_;          // MySQL connection object

    // Begin a transaction
    void beginTransaction()
    {
        if (mysql_query(conn_, "START TRANSACTION"))
        {
            throw std::runtime_error("Failed to begin transaction: " + std::string(mysql_error(conn_)));
        }
    }

    // Commit a transaction
    void commit()
    {
        if (mysql_query(conn_, "COMMIT"))
        {
            throw std::runtime_error("Failed to commit transaction: " + std::string(mysql_error(conn_)));
        }
    }

    // Rollback a transaction
    void rollback()
    {
        if (mysql_query(conn_, "ROLLBACK"))
        {
            throw std::runtime_error("Failed to rollback transaction: " + std::string(mysql_error(conn_)));
        }
    }
};