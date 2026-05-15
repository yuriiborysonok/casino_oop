#pragma once
#include "ILogger.hpp"
#include <iostream>

// Конкретна реалізація логера
class SpdLogger : public ILogger {
public:
    void info(const std::string& message) override {
        std::cout << "[INFO] " << message << std::endl;
    }
    void error(const std::string& message) override {
        std::cerr << "[ERROR] " << message << std::endl;
    }
};
