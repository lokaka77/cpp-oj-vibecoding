#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class JudgeWorkerPool {
public:
    explicit JudgeWorkerPool(std::size_t worker_count);
    ~JudgeWorkerPool();

    void start();
    void stop();

    bool submit(std::function<void()> task);
    std::size_t worker_count() const;

private:
    void worker_loop();

    std::size_t worker_count_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<std::function<void()>> tasks_;
    std::vector<std::thread> workers_;
    bool running_ = false;
};
