#include <gtest/gtest.h>

// Einfacher Mathe-Test
TEST(BasicTest, AdditionWorks) {
    EXPECT_EQ(2 + 2, 4);
}

// String-Test (Standard C++)
TEST(BasicTest, StringCompare) {
    std::string a = "Ghost";
    std::string b = "Ghost";

    EXPECT_EQ(a, b);
}

// Vector-Test
TEST(BasicTest, VectorSize) {
    std::vector<int> v = {1, 2, 3};
    EXPECT_EQ(v.size(), 3);
}