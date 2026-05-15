#pragma once
#include <string>
#include <utility>

// Абстракція бази даних
class IDatabase {
public:
    virtual ~IDatabase() = default;
    virtual void saveTransaction(int userId, double amount, const std::string& type, const std::string& currency) = 0;
    
    // Auth
    virtual int registerUser(const std::string& username, const std::string& password) = 0;
    virtual int authenticateUser(const std::string& username, const std::string& password) = 0;
    virtual int socialLoginUser(const std::string& email, const std::string& provider) = 0;
    virtual std::pair<double, double> getBalances(int userId) = 0;
};
