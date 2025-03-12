#include <iostream>
#include <mutex>
#include <condition_variable>
#include <future>
#include "utils/ThreadPool.hpp"

// 全局变量用于同步
std::mutex mtx;
std::condition_variable cv;
int turn = 1; // 1表示轮到打印1，2表示轮到打印2

// 打印函数
void printNumber(int num, int targetTurn) {
    for (int i = 0; i < 5; ++i) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [targetTurn] { return turn == targetTurn; }); // 等待轮到当前任务
        std::cout << num << std::endl;
        turn = (targetTurn == 1) ? 2 : 1; // 切换为另一个任务
        cv.notify_all(); // 通知另一个线程
    }
}

int main() {
    // 创建一个包含2个线程的线程池
    ThreadPool pool(2);

    // 提交任务到线程池
    auto future1 = pool.enqueue([] { printNumber(1, 1); });
    auto future2 = pool.enqueue([] { printNumber(2, 2); });

    // 等待任务完成
    future1.get();
    future2.get();

    return 0;
}