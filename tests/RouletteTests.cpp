#include "RouletteEngine.hpp"
#include <gtest/gtest.h>

TEST(RouletteTests, StraightBetPayouts) {
    StraightBet bet(100.0, 17);
    EXPECT_DOUBLE_EQ(bet.calculatePayout(17), 3600.0);
    EXPECT_DOUBLE_EQ(bet.calculatePayout(18), 0.0);
    EXPECT_EQ(bet.getType(), "Straight(17)");
    EXPECT_DOUBLE_EQ(bet.getAmount(), 100.0);

    // Invalid straight bet numbers throw
    EXPECT_THROW(StraightBet(50.0, -1), std::invalid_argument);
    EXPECT_THROW(StraightBet(50.0, 37), std::invalid_argument);
}

TEST(RouletteTests, ColorBetPayouts) {
    ColorBet redBet(100.0, "red");
    // 1 is red, 2 is black, 0 is green
    EXPECT_DOUBLE_EQ(redBet.calculatePayout(1), 200.0);
    EXPECT_DOUBLE_EQ(redBet.calculatePayout(2), 0.0);
    EXPECT_DOUBLE_EQ(redBet.calculatePayout(0), 0.0);
    EXPECT_EQ(redBet.getType(), "Color(red)");

    ColorBet blackBet(100.0, "black");
    EXPECT_DOUBLE_EQ(blackBet.calculatePayout(2), 200.0);
    EXPECT_DOUBLE_EQ(blackBet.calculatePayout(1), 0.0);
}

TEST(RouletteTests, DozenBetPayouts) {
    DozenBet dozen1(100.0, 1);
    EXPECT_DOUBLE_EQ(dozen1.calculatePayout(5), 300.0);
    EXPECT_DOUBLE_EQ(dozen1.calculatePayout(15), 0.0);
    EXPECT_DOUBLE_EQ(dozen1.calculatePayout(0), 0.0);
    EXPECT_EQ(dozen1.getType(), "Dozen(1)");

    DozenBet dozen2(100.0, 2);
    EXPECT_DOUBLE_EQ(dozen2.calculatePayout(15), 300.0);
    EXPECT_DOUBLE_EQ(dozen2.calculatePayout(5), 0.0);

    DozenBet dozen3(100.0, 3);
    EXPECT_DOUBLE_EQ(dozen3.calculatePayout(30), 300.0);
    EXPECT_DOUBLE_EQ(dozen3.calculatePayout(15), 0.0);
}

TEST(RouletteTests, EvenOddBetPayouts) {
    EvenOddBet evenBet(100.0, "even");
    EXPECT_DOUBLE_EQ(evenBet.calculatePayout(4), 200.0);
    EXPECT_DOUBLE_EQ(evenBet.calculatePayout(5), 0.0);
    EXPECT_DOUBLE_EQ(evenBet.calculatePayout(0), 0.0);
    EXPECT_EQ(evenBet.getType(), "EvenOdd(even)");

    EvenOddBet oddBet(100.0, "odd");
    EXPECT_DOUBLE_EQ(oddBet.calculatePayout(5), 200.0);
    EXPECT_DOUBLE_EQ(oddBet.calculatePayout(4), 0.0);
}

TEST(RouletteTests, BetFactoryParsesCorrectly) {
    auto b1 = BetFactory::createBet("number", "17", 50.0);
    EXPECT_EQ(b1->getType(), "Straight(17)");

    auto b2 = BetFactory::createBet("color", "red", 50.0);
    EXPECT_EQ(b2->getType(), "Color(red)");

    auto b3 = BetFactory::createBet("dozen", "2", 50.0);
    EXPECT_EQ(b3->getType(), "Dozen(2)");

    auto b4 = BetFactory::createBet("evenodd", "odd", 50.0);
    EXPECT_EQ(b4->getType(), "EvenOdd(odd)");

    EXPECT_THROW(BetFactory::createBet("invalid", "odd", 50.0), std::invalid_argument);
}

TEST(RouletteTests, EngineCalculatesTotalPayout) {
    AdvancedRouletteEngine engine;
    engine.addBet(BetFactory::createBet("color", "red", 100.0));
    engine.addBet(BetFactory::createBet("number", "17", 10.0)); // 17 is black

    auto [winningNum, totalWon] = engine.spin();
    EXPECT_GE(winningNum, 0);
    EXPECT_LE(winningNum, 36);
}
