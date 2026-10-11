#include <gtest/gtest.h>

#include "session.h"

#include <cstdint>
#include <string>

TEST(SessionManagerTest, CreatesNonEmptyToken) {
    SessionManager sessions;
    std::string token = sessions.create(42);

    EXPECT_FALSE(token.empty());
}

TEST(SessionManagerTest, ResolvesValidToken) {
    SessionManager sessions;
    std::string token = sessions.create(42);
    std::uint64_t user_id = 0;

    ASSERT_FALSE(token.empty());
    EXPECT_TRUE(sessions.get(token, user_id));
    EXPECT_EQ(user_id, 42);
}

TEST(SessionManagerTest, RejectsUnknownToken) {
    SessionManager sessions;
    std::uint64_t user_id = 0;

    EXPECT_FALSE(sessions.get("unknown-token", user_id));
}

TEST(SessionManagerTest, RemoveInvalidatesToken) {
    SessionManager sessions;
    std::string token = sessions.create(42);
    std::uint64_t user_id = 0;

    sessions.remove(token);
    EXPECT_FALSE(sessions.get(token, user_id));
}

TEST(SessionManagerTest, KeepsSessionsIndependent) {
    SessionManager sessions;
    std::string first = sessions.create(1);
    std::string second = sessions.create(2);
    std::uint64_t user_id = 0;

    ASSERT_TRUE(sessions.get(first, user_id));
    EXPECT_EQ(user_id, 1);

    ASSERT_TRUE(sessions.get(second, user_id));
    EXPECT_EQ(user_id, 2);
}
