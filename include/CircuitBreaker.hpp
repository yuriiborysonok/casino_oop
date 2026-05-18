#pragma once
#include <mutex>
#include <chrono>
#include <string>

class CircuitBreaker {
public:
    enum State { CLOSED, OPEN, HALF_OPEN };

    CircuitBreaker(int failureThreshold = 3, int cooldownSeconds = 10)
        : state(CLOSED), failureCount(0), failureThreshold(failureThreshold), cooldownSeconds(cooldownSeconds) {}

    bool allowRequest() {
        std::lock_guard<std::mutex> lock(mtx);
        if (state == OPEN) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastStateChange).count();
            if (elapsed >= cooldownSeconds) {
                state = HALF_OPEN;
                lastStateChange = now;
                return true;
            }
            return false;
        }
        return true;
    }

    void recordSuccess() {
        std::lock_guard<std::mutex> lock(mtx);
        failureCount = 0;
        state = CLOSED;
    }

    void recordFailure() {
        std::lock_guard<std::mutex> lock(mtx);
        failureCount++;
        if (state == HALF_OPEN || failureCount >= failureThreshold) {
            state = OPEN;
            lastStateChange = std::chrono::steady_clock::now();
        }
    }

    State getState() const {
        return state;
    }

    std::string getStateString() const {
        switch (state) {
            case CLOSED: return "CLOSED";
            case OPEN: return "OPEN";
            case HALF_OPEN: return "HALF_OPEN";
        }
        return "UNKNOWN";
    }

private:
    State state;
    int failureCount;
    int failureThreshold;
    int cooldownSeconds;
    std::chrono::steady_clock::time_point lastStateChange;
    mutable std::mutex mtx;
};
