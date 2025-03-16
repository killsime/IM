#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>
#include <atomic>
#include <memory>
#include <stdexcept>

class ThreadPool {
public:
    // 获取单例实例
    static ThreadPool& getInstance(size_t minThreads = 4, size_t maxThreads = 16) {
        static ThreadPool instance(minThreads, maxThreads);
        return instance;
    }

    // 提交任务
    template <typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using return_type = decltype(f(args...)); // 使用 decltype 推导返回类型

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex);

            if (stop) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }

            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one();
        adjustThreadCount();
        return res;
    }

    // 禁止拷贝和赋值
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    // 构造函数
    ThreadPool(size_t minThreads, size_t maxThreads)
        : minThreads(minThreads), maxThreads(maxThreads), stop(false) {
        for (size_t i = 0; i < minThreads; ++i) {
            workers.emplace_back([this] { workerThread(); });
        }
    }

    // 析构函数
    ~ThreadPool() {
        stop = true;
        condition.notify_all();
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    // 工作线程逻辑
    void workerThread() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                condition.wait(lock, [this] {
                    return stop || !tasks.empty();
                });

                if (stop && tasks.empty()) {
                    return;
                }

                task = std::move(tasks.front());
                tasks.pop();
            }
            task();
        }
    }

    // 动态调整线程数量
    void adjustThreadCount() {
        std::unique_lock<std::mutex> lock(queueMutex);
        size_t currentThreads = workers.size();
        size_t pendingTasks = tasks.size();

        if (pendingTasks > currentThreads && currentThreads < maxThreads) {
            // 增加线程
            workers.emplace_back([this] { workerThread(); });
        } else if (pendingTasks < currentThreads && currentThreads > minThreads) {
            // 减少线程
            for (size_t i = 0; i < currentThreads - minThreads; ++i) {
                if (workers.back().joinable()) {
                    workers.back().detach();
                }
                workers.pop_back();
            }
        }
    }

    size_t minThreads; // 最小线程数
    size_t maxThreads; // 最大线程数
    std::vector<std::thread> workers; // 工作线程
    std::queue<std::function<void()>> tasks; // 任务队列
    std::mutex queueMutex; // 任务队列的互斥锁
    std::condition_variable condition; // 条件变量
    std::atomic<bool> stop; // 线程池是否停止
};

#endif // THREADPOOL_HPP