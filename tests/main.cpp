#include <gtest/gtest.h>

// Базовий тест для перевірки того, що GTest налаштовано правильно
TEST(EnvironmentTest, GTestIsWorking) {
    EXPECT_EQ(1, 1);
}

// Завдяки лінкуванню з gtest_main, нам не потрібно писати функцію main()
