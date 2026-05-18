#pragma once
#include <string>
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <ctime>

// Represents a single playing card
struct Card {
    std::string suit; // Hearts, Diamonds, Clubs, Spades
    std::string value; // "2"-"10", "J", "Q", "K", "A"
    int points;

    Card(std::string s, std::string v) : suit(std::move(s)), value(std::move(v)) {
        if (value == "J" || value == "Q" || value == "K") {
            points = 10;
        } else if (value == "A") {
            points = 11; // Handled dynamically in getScore()
        } else {
            points = std::stoi(value);
        }
    }
};

// Represents a deck of 52 cards
class Deck {
    std::vector<Card> cards;
public:
    Deck() {
        std::vector<std::string> suits = {"♥", "♦", "♣", "♠"};
        std::vector<std::string> values = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A"};
        for (const auto& suit : suits) {
            for (const auto& val : values) {
                cards.emplace_back(suit, val);
            }
        }
    }

    void shuffle() {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(cards.begin(), cards.end(), g);
    }

    Card draw() {
        if (cards.empty()) {
            // Re-create deck if depleted
            *this = Deck();
            shuffle();
        }
        Card c = cards.back();
        cards.pop_back();
        return c;
    }
};

// Blackjack Logic Engine
class BlackjackEngine {
public:
    // Helper to calculate total score of a hand, handling Aces dynamically
    static int calculateScore(const std::vector<Card>& hand) {
        int score = 0;
        int acesCount = 0;
        for (const auto& card : hand) {
            score += card.points;
            if (card.value == "A") {
                acesCount++;
            }
        }
        while (score > 21 && acesCount > 0) {
            score -= 10; // Change Ace value from 11 to 1
            acesCount--;
        }
        return score;
    }

    // Determine the result status of a finished round
    // Returns: "win", "lose", "push", "blackjack"
    static std::string determineResult(int playerScore, int dealerScore) {
        if (playerScore > 21) return "lose"; // Player busted
        if (dealerScore > 21) return "win";  // Dealer busted
        
        if (playerScore > dealerScore) {
            if (playerScore == 21 && dealerScore != 21) return "win"; // Can be blackjack
            return "win";
        }
        if (playerScore < dealerScore) return "lose";
        return "push"; // Draw
    }
};
