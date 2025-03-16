#include <iostream>
#include <string>
#include <future>
#include "utils/ThreadPool.hpp"

// 示例任务函数 1：返回整数
int add(int a, int b) {
    std::this_thread::sleep_for(std::chrono::seconds(1)); // 模拟耗时操作
    return a + b;
}

// 示例任务函数 2：返回字符串
std::string concatenate(const std::string& str1, const std::string& str2) {
    std::this_thread::sleep_for(std::chrono::seconds(1)); // 模拟耗时操作
    return str1 + str2;
}

// 示例任务函数 3：返回浮点数
double multiply(double a, double b) {
    std::this_thread::sleep_for(std::chrono::seconds(1)); // 模拟耗时操作
    return a * b;
}

int main() {
    // 创建一个包含 4 个线程的线程池

    // 提交多个不同类型的任务到线程池
    auto future1 = pool.enqueue(add, 10, 20);          // 提交 add 任务
    auto future2 = pool.enqueue(concatenate, "Hello, ", "World!"); // 提交 concatenate 任务
    auto future3 = pool.enqueue(multiply, 3.14, 2.0);  // 提交 multiply 任务

    // 获取任务返回值
    int result1 = future1.get();
    std::string result2 = future2.get();
    double result3 = future3.get();

    // 输出结果
    std::cout << "Result of add: " << result1 << std::endl;
    std::cout << "Result of concatenate: " << result2 << std::endl;
    std::cout << "Result of multiply: " << result3 << std::endl;

    return 0;
}