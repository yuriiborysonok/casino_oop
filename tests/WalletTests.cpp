#include <gtest/gtest.h>
#include "Wallet.hpp"
#include <stdexcept>

// Позитивний сценарій: Новий гаманець має нульовий баланс
TEST(WalletTests, InitialBalanceIsZero) {
    Wallet wallet;
    EXPECT_EQ(wallet.getBalance(), 0.0);
}

// Позитивний сценарій: Поповнення балансу
TEST(WalletTests, DepositIncreasesBalance) {
    Wallet wallet;
    wallet.deposit(100.50);
    EXPECT_EQ(wallet.getBalance(), 100.50);
}

// Позитивний сценарій: Зняття коштів
TEST(WalletTests, WithdrawDecreasesBalance) {
    Wallet wallet;
    wallet.deposit(100.0);
    wallet.withdraw(40.0);
    EXPECT_EQ(wallet.getBalance(), 60.0);
}

// Негативний сценарій: Спроба поповнити на від'ємну суму викидає помилку
TEST(WalletTests, DepositNegativeAmountThrowsException) {
    Wallet wallet;
    EXPECT_THROW(wallet.deposit(-50.0), std::invalid_argument);
}

// Негативний сценарій: Спроба зняти більше, ніж є на балансі
TEST(WalletTests, WithdrawMoreThanBalanceThrowsException) {
    Wallet wallet;
    wallet.deposit(50.0);
    EXPECT_THROW(wallet.withdraw(100.0), std::logic_error);
}
