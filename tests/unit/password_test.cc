#include <gtest/gtest.h>

#include "password.h"

#include <string>

TEST(PasswordTest, HashesWithArgon2id) {
    std::string hash = hash_password("secret123");

    EXPECT_FALSE(hash.empty());
    EXPECT_EQ(hash.rfind("$argon2id$", 0), 0);
}

TEST(PasswordTest, VerifiesCorrectPassword) {
    std::string hash = hash_password("secret123");

    ASSERT_FALSE(hash.empty());
    EXPECT_TRUE(verify_password(hash, "secret123"));
}

TEST(PasswordTest, RejectsWrongPassword) {
    std::string hash = hash_password("secret123");

    ASSERT_FALSE(hash.empty());
    EXPECT_FALSE(verify_password(hash, "wrong-password"));
}

TEST(PasswordTest, RejectsEmptyHash) {
    EXPECT_FALSE(verify_password("", "secret123"));
}
