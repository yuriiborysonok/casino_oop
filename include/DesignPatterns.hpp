#pragma once
// 1. Поведінковий патерн "Стратегія"
class IPayoutStrategy {
public:
  virtual ~IPayoutStrategy() = default;
  // Обчислює суму виграшу на основі ставки
  virtual double calculatePayout(double betAmount) const = 0;
};

class ColorPayoutStrategy : public IPayoutStrategy {
public:
  double calculatePayout(double betAmount) const override {
    return betAmount * 2.0;
  }
};

class NumberPayoutStrategy : public IPayoutStrategy {
public:
  double calculatePayout(double betAmount) const override {
    return betAmount * 36.0;
  }
};

// 2. Патерн декоратор
#include <memory>

class PayoutDecorator : public IPayoutStrategy {
protected:
  std::unique_ptr<IPayoutStrategy> wrappedStrategy;

public:
  explicit PayoutDecorator(std::unique_ptr<IPayoutStrategy> strategy)
      : wrappedStrategy(std::move(strategy)) {}

  double calculatePayout(double betAmount) const override {
    return wrappedStrategy->calculatePayout(betAmount);
  }
};

class HappyHourBonusDecorator : public PayoutDecorator {
public:
  explicit HappyHourBonusDecorator(std::unique_ptr<IPayoutStrategy> strategy)
      : PayoutDecorator(std::move(strategy)) {}

  double calculatePayout(double betAmount) const override {
    // Базовий виграш + 50% бонус
    return PayoutDecorator::calculatePayout(betAmount) * 1.5;
  }
};
// 3. Фабричний метод
#include <stdexcept>
#include <string>

class IGame {
public:
  virtual ~IGame() = default;
  virtual std::string getName() const = 0;
};

class RouletteGame : public IGame {
public:
  std::string getName() const override { return "Roulette"; }
};

class SlotMachineGame : public IGame {
public:
  std::string getName() const override { return "SlotMachine"; }
};

class GameFactory {
public:
  // Фабричний метод для створення ігор
  static std::unique_ptr<IGame> createGame(const std::string &type) {
    if (type == "Roulette")
      return std::make_unique<RouletteGame>();
    if (type == "Slots")
      return std::make_unique<SlotMachineGame>();
    throw std::invalid_argument("Unknown game type");
  }
};
