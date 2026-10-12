#include <gtest/gtest.h>

#include "output_comparer.h"

TEST(OutputComparerTest, MatchesExactOutput) {
    EXPECT_TRUE(OutputComparer::matches("42\n", "42\n"));
}

TEST(OutputComparerTest, IgnoresTrailingSpaces) {
    EXPECT_TRUE(OutputComparer::matches("42   \t", "42"));
    EXPECT_TRUE(OutputComparer::matches("42", "42  \n"));
}

TEST(OutputComparerTest, IgnoresTrailingNewlines) {
    EXPECT_TRUE(OutputComparer::matches("42\n\n", "42\n"));
    EXPECT_TRUE(OutputComparer::matches("42\r\n", "42\n"));
}

TEST(OutputComparerTest, DoesNotIgnoreInternalWhitespace) {
    EXPECT_FALSE(OutputComparer::matches("4 2\n", "42\n"));
}

TEST(OutputComparerTest, DetectsDifferentContent) {
    EXPECT_FALSE(OutputComparer::matches("41\n", "42\n"));
}
