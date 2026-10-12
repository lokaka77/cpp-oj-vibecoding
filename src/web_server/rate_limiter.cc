#include "rate_limiter.h"

SubmissionRateLimiter::SubmissionRateLimiter(
    std::chrono::milliseconds interval)
    : interval_(interval) {}

bool SubmissionRateLimiter::allow(std::uint64_t user_id) {
    if (interval_.count() <= 0) {
        return true;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const auto now = std::chrono::steady_clock::now();
    auto iterator = last_submission_.find(user_id);
    if (iterator == last_submission_.end() ||
        now - iterator->second >= interval_) {
        last_submission_[user_id] = now;
        return true;
    }

    return false;
}

std::chrono::milliseconds SubmissionRateLimiter::retry_after(
    std::uint64_t user_id) const {
    if (interval_.count() <= 0) {
        return std::chrono::milliseconds(0);
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto iterator = last_submission_.find(user_id);
    if (iterator == last_submission_.end()) {
        return std::chrono::milliseconds(0);
    }

    const auto elapsed = std::chrono::steady_clock::now() - iterator->second;
    if (elapsed >= interval_) {
        return std::chrono::milliseconds(0);
    }

    return std::chrono::duration_cast<std::chrono::milliseconds>(
        interval_ - elapsed);
}
