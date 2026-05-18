#include "CircuitBreaker.hpp"
#include <gtest/gtest.h>
#include <thread>

TEST(CircuitBreakerTests, InitialStateIsClosed) {
    CircuitBreaker breaker(3, 1); // 3 failures threshold, 1 second cooldown
    EXPECT_EQ(breaker.getState(), CircuitBreaker::CLOSED);
    EXPECT_EQ(breaker.getStateString(), "CLOSED");
    EXPECT_TRUE(breaker.allowRequest());
}

TEST(CircuitBreakerTests, TripleFailureTripsToOpen) {
    CircuitBreaker breaker(3, 1);
    
    // 1st failure
    breaker.recordFailure();
    EXPECT_EQ(breaker.getState(), CircuitBreaker::CLOSED);
    EXPECT_TRUE(breaker.allowRequest());

    // 2nd failure
    breaker.recordFailure();
    EXPECT_EQ(breaker.getState(), CircuitBreaker::CLOSED);
    EXPECT_TRUE(breaker.allowRequest());

    // 3rd failure - should trip to OPEN
    breaker.recordFailure();
    EXPECT_EQ(breaker.getState(), CircuitBreaker::OPEN);
    EXPECT_EQ(breaker.getStateString(), "OPEN");
    EXPECT_FALSE(breaker.allowRequest());
}

TEST(CircuitBreakerTests, SuccessResetsFailureCount) {
    CircuitBreaker breaker(3, 1);
    breaker.recordFailure();
    breaker.recordFailure();
    breaker.recordSuccess(); // Reset failures to 0
    
    breaker.recordFailure();
    EXPECT_EQ(breaker.getState(), CircuitBreaker::CLOSED);
    EXPECT_TRUE(breaker.allowRequest());
}

TEST(CircuitBreakerTests, CooldownTransitionsToHalfOpen) {
    CircuitBreaker breaker(2, 1); // 2 failures, 1 second cooldown
    breaker.recordFailure();
    breaker.recordFailure(); // Trips to OPEN
    
    EXPECT_EQ(breaker.getState(), CircuitBreaker::OPEN);
    EXPECT_FALSE(breaker.allowRequest());

    // Sleep for 1.1 seconds to trigger cooldown expiration
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    // Request check should transition it to HALF_OPEN
    EXPECT_TRUE(breaker.allowRequest());
    EXPECT_EQ(breaker.getState(), CircuitBreaker::HALF_OPEN);
    EXPECT_EQ(breaker.getStateString(), "HALF_OPEN");
}

TEST(CircuitBreakerTests, HalfOpenSuccessClosesCircuit) {
    CircuitBreaker breaker(2, 1);
    breaker.recordFailure();
    breaker.recordFailure(); // OPEN
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    EXPECT_TRUE(breaker.allowRequest()); // Transitions to HALF_OPEN
    
    breaker.recordSuccess(); // Transitions back to CLOSED
    EXPECT_EQ(breaker.getState(), CircuitBreaker::CLOSED);
    EXPECT_TRUE(breaker.allowRequest());
}

TEST(CircuitBreakerTests, HalfOpenFailureTripsToOpenImmediately) {
    CircuitBreaker breaker(2, 1);
    breaker.recordFailure();
    breaker.recordFailure(); // OPEN
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    EXPECT_TRUE(breaker.allowRequest()); // Transitions to HALF_OPEN
    
    breaker.recordFailure(); // Re-trips immediately on first failure
    EXPECT_EQ(breaker.getState(), CircuitBreaker::OPEN);
    EXPECT_FALSE(breaker.allowRequest());
}
