#include "Wallet.hpp"

Wallet::Wallet() : balance(0.0) {}

double Wallet::getBalance() const {
    return balance;
}

void Wallet::deposit(double amount) {
    if (amount <= 0) {
        throw std::invalid_argument("Deposit amount must be positive.");
    }
    balance += amount;
}

void Wallet::withdraw(double amount) {
    if (amount <= 0) {
        throw std::invalid_argument("Withdraw amount must be positive.");
    }
    if (amount > balance) {
        throw std::logic_error("Insufficient funds.");
    }
    balance -= amount;
}
