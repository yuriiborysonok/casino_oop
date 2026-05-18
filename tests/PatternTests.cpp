#include "DesignPatterns.hpp"
#include <gtest/gtest.h>

// Тест для Strategy Pattern
TEST(PatternTests, StrategyCalculatesCorrectly) {
  ColorPayoutStrategy colorStrategy;
  EXPECT_EQ(colorStrategy.calculatePayout(100.0), 200.0);
  NumberPayoutStrategy numberStrategy;
  EXPECT_EQ(numberStrategy.calculatePayout(10.0), 360.0);
}

// Тест для Decorator Pattern
TEST(PatternTests, DecoratorAddsBonusCorrectly) {
  auto baseStrategy = std::make_unique<ColorPayoutStrategy>();
  HappyHourBonusDecorator bonusStrategy(std::move(baseStrategy));

  // Базова ставка 100 * 2 = 200. З бонусом x1.5 має бути 300
  EXPECT_EQ(bonusStrategy.calculatePayout(100.0), 300.0);
}

// Тест для Factory Pattern
TEST(PatternTests, FactoryCreatesCorrectGameTypes) {
  auto game1 = GameFactory::createGame("Roulette");
  EXPECT_EQ(game1->getName(), "Roulette");

  auto game2 = GameFactory::createGame("Slots");
  EXPECT_EQ(game2->getName(), "SlotMachine");
}

// Тест для Factory Pattern
TEST(PatternTests, FactoryThrowsOnUnknownGame) {
  EXPECT_THROW(GameFactory::createGame("Poker"), std::invalid_argument);
}
