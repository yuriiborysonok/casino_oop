#include "BlackjackEngine.hpp"
#include <gtest/gtest.h>

TEST(BlackjackTests, CardValuesCalculatedCorrectly) {
    Card two("♥", "2");
    EXPECT_EQ(two.points, 2);

    Card ten("♦", "10");
    EXPECT_EQ(ten.points, 10);

    Card jack("♣", "J");
    EXPECT_EQ(jack.points, 10);

    Card queen("♠", "Q");
    EXPECT_EQ(queen.points, 10);

    Card king("♥", "K");
    EXPECT_EQ(king.points, 10);

    Card ace("♦", "A");
    EXPECT_EQ(ace.points, 11);
}

TEST(BlackjackTests, HandScoreCalculatedCorrectly) {
    std::vector<Card> hand1 = {Card("♥", "10"), Card("♣", "5")};
    EXPECT_EQ(BlackjackEngine::calculateScore(hand1), 15);

    std::vector<Card> hand2 = {Card("♥", "A"), Card("♣", "A")};
    // Second Ace value reduces to 1: 11 + 1 = 12
    EXPECT_EQ(BlackjackEngine::calculateScore(hand2), 12);

    std::vector<Card> hand3 = {Card("♥", "10"), Card("♣", "A")};
    EXPECT_EQ(BlackjackEngine::calculateScore(hand3), 21);

    std::vector<Card> hand4 = {Card("♥", "10"), Card("♣", "10"), Card("♦", "A")};
    // Ace value is reduced to 1 to avoid bust: 10 + 10 + 1 = 21
    EXPECT_EQ(BlackjackEngine::calculateScore(hand4), 21);
}

TEST(BlackjackTests, DetermineResultCorrectly) {
    EXPECT_EQ(BlackjackEngine::determineResult(22, 18), "lose"); // Player busted
    EXPECT_EQ(BlackjackEngine::determineResult(18, 22), "win");  // Dealer busted
    EXPECT_EQ(BlackjackEngine::determineResult(20, 18), "win");  // Player score higher
    EXPECT_EQ(BlackjackEngine::determineResult(18, 20), "lose"); // Dealer score higher
    EXPECT_EQ(BlackjackEngine::determineResult(18, 18), "push"); // Tie
}

TEST(BlackjackTests, DeckShufflesAndDraws) {
    Deck deck;
    Card c = deck.draw();
    EXPECT_FALSE(c.suit.empty());
    EXPECT_FALSE(c.value.empty());

    // Empty deck recreate simulation: draw 60 cards
    for (int i = 0; i < 60; ++i) {
        deck.draw();
    }
    // Recreate card count should draw fine
    Card c2 = deck.draw();
    EXPECT_FALSE(c2.suit.empty());
}
