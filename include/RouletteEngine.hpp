#pragma once
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <cstdlib>

// Base class for a Roulette Bet
class RouletteBet {
protected:
    double amount;
public:
    explicit RouletteBet(double amt) : amount(amt) {}
    virtual ~RouletteBet() = default;

    virtual double calculatePayout(int winningNumber) const = 0;
    virtual std::string getType() const = 0;
    double getAmount() const { return amount; }
};

// 1. Straight Bet (Single Number)
class StraightBet : public RouletteBet {
    int targetNumber;
public:
    StraightBet(double amt, int num) : RouletteBet(amt), targetNumber(num) {
        if (num < 0 || num > 36) throw std::invalid_argument("Invalid roulette number");
    }
    double calculatePayout(int winningNumber) const override {
        return (winningNumber == targetNumber) ? (amount * 36.0) : 0.0;
    }
    std::string getType() const override { return "Straight(" + std::to_string(targetNumber) + ")"; }
};

// 2. Color Bet (Red/Black)
class ColorBet : public RouletteBet {
    std::string color;
    bool isRed(int number) const {
        int reds[] = {1,3,5,7,9,12,14,16,18,19,21,23,25,27,30,32,34,36};
        for(int r : reds) {
            if (number == r) return true;
        }
        return false;
    }
public:
    ColorBet(double amt, const std::string& col) : RouletteBet(amt), color(col) {}
    double calculatePayout(int winningNumber) const override {
        if (winningNumber == 0) return 0.0;
        bool resultRed = isRed(winningNumber);
        if (color == "red" && resultRed) return amount * 2.0;
        if (color == "black" && !resultRed) return amount * 2.0;
        return 0.0;
    }
    std::string getType() const override { return "Color(" + color + ")"; }
};

// 3. Dozen Bet (1st 12, 2nd 12, 3rd 12)
class DozenBet : public RouletteBet {
    int dozenIndex; 
public:
    DozenBet(double amt, int index) : RouletteBet(amt), dozenIndex(index) {}
    double calculatePayout(int winningNumber) const override {
        if (winningNumber == 0) return 0.0;
        if (dozenIndex == 1 && winningNumber >= 1 && winningNumber <= 12) return amount * 3.0;
        if (dozenIndex == 2 && winningNumber >= 13 && winningNumber <= 24) return amount * 3.0;
        if (dozenIndex == 3 && winningNumber >= 25 && winningNumber <= 36) return amount * 3.0;
        return 0.0;
    }
    std::string getType() const override { return "Dozen(" + std::to_string(dozenIndex) + ")"; }
};

// 4. Even/Odd Bet
class EvenOddBet : public RouletteBet {
    std::string type;
public:
    EvenOddBet(double amt, const std::string& t) : RouletteBet(amt), type(t) {}
    double calculatePayout(int winningNumber) const override {
        if (winningNumber == 0) return 0.0;
        if (type == "even" && winningNumber % 2 == 0) return amount * 2.0;
        if (type == "odd" && winningNumber % 2 != 0) return amount * 2.0;
        return 0.0;
    }
    std::string getType() const override { return "EvenOdd(" + type + ")"; }
};

// Factory for parsing bets from JSON
class BetFactory {
public:
    static std::unique_ptr<RouletteBet> createBet(const std::string& type, const std::string& value, double amount) {
        if (type == "number") return std::make_unique<StraightBet>(amount, std::stoi(value));
        if (type == "color") return std::make_unique<ColorBet>(amount, value);
        if (type == "dozen") return std::make_unique<DozenBet>(amount, std::stoi(value));
        if (type == "evenodd") return std::make_unique<EvenOddBet>(amount, value);
        throw std::invalid_argument("Unknown bet type: " + type);
    }
};

class AdvancedRouletteEngine {
    std::vector<std::unique_ptr<RouletteBet>> bets;
public:
    void addBet(std::unique_ptr<RouletteBet> bet) {
        bets.push_back(std::move(bet));
    }
    
    // Returns {winning_number, total_payout}
    std::pair<int, double> spin() {
        int winningNumber = std::rand() % 37;
        double totalPayout = 0;
        for (const auto& bet : bets) {
            totalPayout += bet->calculatePayout(winningNumber);
        }
        bets.clear();
        return {winningNumber, totalPayout};
    }
};
