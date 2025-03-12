#include "utils/ThreadPool.hpp"


int main() {
    // 创建一个包含4个线程的线程池
    ThreadPool pool(4);

    // 提交一些任务到线程池
    auto result1 = pool.enqueue([](int a, int b) { return a + b; }, 1, 2);
    auto result2 = pool.enqueue([](int a, int b) { return a * b; }, 3, 4);
    auto result3 = pool.enqueue([]() { std::this_thread::sleep_for(std::chrono::seconds(1)); return 42; });

    // 获取任务的结果
    std::cout << "Result of 1 + 2: " << result1.get() << std::endl;
    std::cout << "Result of 3 * 4: " << result2.get() << std::endl;
    std::cout << "Result of sleep task: " << result3.get() << std::endl;

    return 0;
}