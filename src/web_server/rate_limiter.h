#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <unordered_map>

class SubmissionRateLimiter {
public:
    explicit SubmissionRateLimiter(std::chrono::milliseconds interval);

    bool allow(std::uint64_t user_id);
    std::chrono::milliseconds retry_after(std::uint64_t user_id) const;

private:
    std::chrono::milliseconds interval_;
    mutable std::mutex mutex_;
    std::unordered_map<std::uint64_t,
                       std::chrono::steady_clock::time_point>
        last_submission_;
};
