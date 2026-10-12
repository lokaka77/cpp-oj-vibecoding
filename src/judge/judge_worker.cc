#include "judge_worker.h"

#include <algorithm>
#include <utility>

JudgeWorkerPool::JudgeWorkerPool(std::size_t worker_count)
    : worker_count_(std::max<std::size_t>(1, worker_count)) {}

JudgeWorkerPool::~JudgeWorkerPool() {
    stop();
}

void JudgeWorkerPool::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_) {
        return;
    }

    running_ = true;
    workers_.reserve(worker_count_);
    for (std::size_t index = 0; index < worker_count_; ++index) {
        workers_.emplace_back([this]() { worker_loop(); });
    }
}

void JudgeWorkerPool::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_) {
            return;
        }
        running_ = false;
    }

    condition_.notify_all();
    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();

    std::lock_guard<std::mutex> lock(mutex_);
    while (!tasks_.empty()) {
        tasks_.pop();
    }
}

bool JudgeWorkerPool::submit(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_) {
            return false;
        }
        tasks_.push(std::move(task));
    }

    condition_.notify_one();
    return true;
}

std::size_t JudgeWorkerPool::worker_count() const {
    return worker_count_;
}

void JudgeWorkerPool::worker_loop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [this]() {
                return !running_ || !tasks_.empty();
            });

            if (!running_ && tasks_.empty()) {
                return;
            }

            task = std::move(tasks_.front());
            tasks_.pop();
        }

        task();
    }
}
