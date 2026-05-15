#pragma once
#include "IDatabase.hpp"
#include "ILogger.hpp"
#include <memory>
#include <pqxx/pqxx>
#include <iostream>

class PostgresDatabase : public IDatabase {
private:
    std::shared_ptr<ILogger> logger;
    std::string connectionString;

public:
    PostgresDatabase(std::shared_ptr<ILogger> logger, const std::string& connStr) 
        : logger(logger), connectionString(connStr) {
        
        try {
            // Підключення до БД та автоматичне створення таблиць
            pqxx::connection conn(connectionString);
            if (conn.is_open()) {
                this->logger->info("Connected to REAL PostgreSQL DB: " + std::string(conn.dbname()));
                
                pqxx::work W(conn);
                
                // 1. Створюємо таблицю гравців (Users)
                W.exec(
                    "CREATE TABLE IF NOT EXISTS users ("
                    "user_id SERIAL PRIMARY KEY, "
                    "username VARCHAR(50) UNIQUE NOT NULL, "
                    "password_hash VARCHAR(255) NOT NULL, "
                    "balance_gold NUMERIC(15, 2) NOT NULL DEFAULT 75000.0, "
                    "balance_sweep NUMERIC(15, 2) NOT NULL DEFAULT 2.0"
                    ");"
                );
                
                // 2. Створюємо таблицю транзакцій (Transactions)
                W.exec(
                    "CREATE TABLE IF NOT EXISTS transactions ("
                    "transaction_id SERIAL PRIMARY KEY, "
                    "user_id INT REFERENCES users(user_id), "
                    "amount NUMERIC(15, 2) NOT NULL, "
                    "currency VARCHAR(10) NOT NULL DEFAULT 'gold', "
                    "type VARCHAR(50) NOT NULL, "
                    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
                    ");"
                );

                // Safe Migration: Add new columns if they don't exist
                try {
                    W.exec("ALTER TABLE users ADD COLUMN IF NOT EXISTS balance_gold NUMERIC(15, 2) DEFAULT 75000.0;");
                    W.exec("ALTER TABLE users ADD COLUMN IF NOT EXISTS balance_sweep NUMERIC(15, 2) DEFAULT 2.0;");
                    W.exec("ALTER TABLE transactions ADD COLUMN IF NOT EXISTS currency VARCHAR(10) DEFAULT 'gold';");
                } catch (...) {
                    // Ignore if already exists
                }
                
                W.commit();
                this->logger->info("PostgreSQL Tables created & verified successfully (with Auth support).");
            }
        } catch (const std::exception& e) {
            this->logger->error("Failed to initialize PostgreSQL: " + std::string(e.what()));
        }
    }

    void saveTransaction(int userId, double amount, const std::string& type, const std::string& currency) override {
        try {
            pqxx::connection conn(connectionString);
            pqxx::work W(conn);
            
            // 1. Записуємо історію транзакцій (INSERT)
            W.exec_params(
                "INSERT INTO transactions (user_id, amount, currency, type) VALUES ($1, $2, $3, $4)",
                userId, amount, currency, type
            );

            // 2. Оновлюємо баланс (UPDATE)
            std::string balance_col = (currency == "sweep") ? "balance_sweep" : "balance_gold";
            
            if (type.find("WIN") != std::string::npos || type.find("INITIAL_DEPOSIT") != std::string::npos) {
                W.exec_params("UPDATE users SET " + balance_col + " = " + balance_col + " + $1 WHERE user_id = $2", amount, userId);
            } else if (type.find("LOSE") != std::string::npos) {
                W.exec_params("UPDATE users SET " + balance_col + " = " + balance_col + " - $1 WHERE user_id = $2", amount, userId);
            }
            
            W.commit();
            logger->info("PostgreSQL [REAL]: Saved transaction -> User: " + std::to_string(userId) + " | Type: " + type + " | Currency: " + currency + " | Amount: " + std::to_string(amount));
        } catch (const std::exception& e) {
            logger->error("Database Error (saveTransaction): " + std::string(e.what()));
        }
    }

    int registerUser(const std::string& username, const std::string& password) override {
        try {
            pqxx::connection conn(connectionString);
            pqxx::work W(conn);
            
            // В реальному житті тут має бути хешування (bcrypt). 
            // Для цілей ООП лабораторної зберігаємо як є (або імітуємо хеш)
            
            pqxx::result res = W.exec_params(
                "INSERT INTO users (username, password_hash, balance_gold, balance_sweep) VALUES ($1, $2, 75000.0, 2.0) RETURNING user_id",
                username, password
            );
            W.commit();
            
            int newUserId = res[0][0].as<int>();
            logger->info("PostgreSQL: Registered new user '" + username + "' with ID=" + std::to_string(newUserId));
            
            // Log initial deposit transactions
            saveTransaction(newUserId, 75000.0, "INITIAL_DEPOSIT", "gold");
            saveTransaction(newUserId, 2.0, "INITIAL_DEPOSIT", "sweep");
            
            return newUserId;
        } catch (const std::exception& e) {
            logger->error("Registration Error: " + std::string(e.what()));
            return -1; // -1 означає помилку (наприклад, такий юзер вже є)
        }
    }

    int authenticateUser(const std::string& username, const std::string& password) override {
        try {
            pqxx::connection conn(connectionString);
            pqxx::work W(conn);
            
            pqxx::result res = W.exec_params(
                "SELECT user_id FROM users WHERE username = $1 AND password_hash = $2",
                username, password
            );
            
            if (res.empty()) return -1; // Неправильний логін або пароль
            
            return res[0][0].as<int>();
        } catch (const std::exception& e) {
            logger->error("Auth Error: " + std::string(e.what()));
            return -1;
        }
    }

    int socialLoginUser(const std::string& email, const std::string& provider) override {
        try {
            pqxx::connection conn(connectionString);
            pqxx::work W(conn);
            
            // Перевіряємо чи такий email вже є в БД
            pqxx::result res = W.exec_params("SELECT user_id FROM users WHERE username = $1", email);
            if (!res.empty()) {
                // Юзер існує -> логінимо
                return res[0][0].as<int>();
            }
            
            // Юзера немає -> автоматично реєструємо
            std::string fakeHash = "OAUTH_" + provider;
            pqxx::result insertRes = W.exec_params(
                "INSERT INTO users (username, password_hash, balance_gold, balance_sweep) VALUES ($1, $2, 75000.0, 2.0) RETURNING user_id",
                email, fakeHash
            );
            W.commit();
            
            int newUserId = insertRes[0][0].as<int>();
            logger->info("PostgreSQL: Registered new OAUTH user '" + email + "' via " + provider);
            
            // Логуємо стартовий бонус
            saveTransaction(newUserId, 75000.0, "INITIAL_DEPOSIT_OAUTH", "gold");
            saveTransaction(newUserId, 2.0, "INITIAL_DEPOSIT_OAUTH", "sweep");
            
            return newUserId;
        } catch (const std::exception& e) {
            logger->error("Social Auth Error: " + std::string(e.what()));
            return -1;
        }
    }

    std::pair<double, double> getBalances(int userId) override {
        try {
            pqxx::connection conn(connectionString);
            pqxx::work W(conn);
            // Default 0.0 if not found, use COALESCE if needed, but we check empty
            pqxx::result res = W.exec_params("SELECT balance_gold, balance_sweep FROM users WHERE user_id = $1", userId);
            if (res.empty()) return {0.0, 0.0};
            return {res[0][0].as<double>(), res[0][1].as<double>()};
        } catch (const std::exception& e) {
            logger->error("GetBalances Error: " + std::string(e.what()));
            return {0.0, 0.0};
        }
    }
};
