#include <gtest/gtest.h>

#include "judge_worker.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace {

bool wait_for(const std::function<bool()>& predicate,
              std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return predicate();
}

}  // namespace

TEST(JudgeWorkerPoolTest, ClampsMinimumWorkerCountToOne) {
    JudgeWorkerPool pool(0);

    EXPECT_EQ(pool.worker_count(), 1U);
}

TEST(JudgeWorkerPoolTest, RejectsTasksBeforeStart) {
    JudgeWorkerPool pool(2);

    EXPECT_FALSE(pool.submit([]() {}));
}

TEST(JudgeWorkerPoolTest, ProcessesAllSubmittedTasks) {
    JudgeWorkerPool pool(2);
    std::atomic<int> completed{0};

    pool.start();
    for (int index = 0; index < 4; ++index) {
        EXPECT_TRUE(pool.submit([&completed]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ++completed;
        }));
    }

    EXPECT_TRUE(wait_for(
        [&completed]() { return completed.load() == 4; },
        std::chrono::milliseconds(1000)));

    pool.stop();
    EXPECT_EQ(completed.load(), 4);
}

TEST(JudgeWorkerPoolTest, DoesNotExceedFixedConcurrency) {
    JudgeWorkerPool pool(2);
    std::atomic<int> active{0};
    std::atomic<int> max_active{0};
    std::atomic<int> completed{0};

    pool.start();
    for (int index = 0; index < 8; ++index) {
        pool.submit([&]() {
            int current = ++active;
            int previous = max_active.load();
            while (current > previous &&
                   !max_active.compare_exchange_weak(previous, current)) {
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            --active;
            ++completed;
        });
    }

    EXPECT_TRUE(wait_for(
        [&completed]() { return completed.load() == 8; },
        std::chrono::milliseconds(1500)));

    pool.stop();
    EXPECT_EQ(completed.load(), 8);
    EXPECT_LE(max_active.load(), 2);
}

TEST(JudgeWorkerPoolTest, RejectsTasksAfterStop) {
    JudgeWorkerPool pool(1);
    pool.start();
    pool.stop();

    EXPECT_FALSE(pool.submit([]() {}));
}
