#include <gtest/gtest.h>

#include "rate_limiter.h"

#include <chrono>
#include <thread>

TEST(SubmissionRateLimiterTest, AllowsFirstSubmission) {
    SubmissionRateLimiter limiter(std::chrono::milliseconds(100));

    EXPECT_TRUE(limiter.allow(1));
}

TEST(SubmissionRateLimiterTest, RejectsImmediateSecondSubmission) {
    SubmissionRateLimiter limiter(std::chrono::milliseconds(100));

    ASSERT_TRUE(limiter.allow(1));
    EXPECT_FALSE(limiter.allow(1));
}

TEST(SubmissionRateLimiterTest, AllowsSubmissionAfterInterval) {
    SubmissionRateLimiter limiter(std::chrono::milliseconds(60));

    ASSERT_TRUE(limiter.allow(1));
    EXPECT_FALSE(limiter.allow(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    EXPECT_TRUE(limiter.allow(1));
}

TEST(SubmissionRateLimiterTest, TracksUsersIndependently) {
    SubmissionRateLimiter limiter(std::chrono::milliseconds(1000));

    EXPECT_TRUE(limiter.allow(1));
    EXPECT_FALSE(limiter.allow(1));
    EXPECT_TRUE(limiter.allow(2));
}

TEST(SubmissionRateLimiterTest, ReportsRemainingWaitTime) {
    SubmissionRateLimiter limiter(std::chrono::milliseconds(100));

    ASSERT_TRUE(limiter.allow(1));
    const auto retry_after = limiter.retry_after(1);
    EXPECT_GT(retry_after.count(), 0);
}
