#include <gtest/gtest.h>
#include <cstdlib>

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    _Exit(result);
}