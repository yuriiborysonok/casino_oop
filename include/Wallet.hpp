#pragma once
#include <stdexcept>

class Wallet {
private:
    double balance;

public:
    Wallet();
    
    double getBalance() const;
    void deposit(double amount);
    void withdraw(double amount);
};
