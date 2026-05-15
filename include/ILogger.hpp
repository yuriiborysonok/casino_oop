#pragma once
#include <string>

// Абстракція логера (SOLID: Dependency Inversion)
class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void info(const std::string& message) = 0;
    virtual void error(const std::string& message) = 0;
};
