#include "DesignPatterns.hpp"
#include "RouletteEngine.hpp"
#include "BlackjackEngine.hpp"
#include "SpdLogger.hpp"
#include "CircuitBreaker.hpp"
#include "httplib.h"
#include <cstdlib>
#include <ctime>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Fault Tolerance & Private Internal Routing Configuration
static CircuitBreaker walletBreaker(3, 10); // 3 failures, 10s cooldown

static std::string getInternalApiKey() {
  const char* env_key = std::getenv("INTERNAL_API_KEY");
  return env_key ? env_key : "super-secret-internal-key-9999";
}

int main() {
  auto logger = std::make_shared<SpdLogger>();
  httplib::Server svr;
  std::srand(std::time(nullptr));

  auto setupCors = [](httplib::Response &res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
  };

  svr.Options("/(.*)", [&](const httplib::Request &, httplib::Response &res) {
    setupCors(res);
    res.status = 204;
  });

  svr.Post("/api/blackjack/deal", [&](const httplib::Request &req, httplib::Response &res) {
    setupCors(res);
    json responseJson;
    try {
      auto body = json::parse(req.body);
      int userId = body["userId"];
      double betAmount = body["betAmount"];
      std::string currency = body.contains("currency") ? body["currency"].get<std::string>() : "gold";

      if (betAmount <= 0) throw std::invalid_argument("Bet amount must be > 0");

      // 1. Check wallet balance via Circuit Breaker
      if (!walletBreaker.allowRequest()) {
        res.status = 503;
        res.set_content(json({{"error", "Wallet Service temporarily unavailable (Circuit Breaker OPEN)"}}).dump(), "application/json");
        return;
      }

      const char *wallet_host = std::getenv("WALLET_HOST");
      std::string whost = wallet_host ? wallet_host : "casino-wallet.fly.dev";
      std::string url = whost;
      if (url.find("://") == std::string::npos) {
        url = "https://" + url;
      }
      httplib::Client cli(url);
      cli.set_connection_timeout(10, 0);
      cli.enable_server_certificate_verification(false);

      httplib::Headers headers = {
        {"X-Internal-Key", getInternalApiKey()}
      };

      auto bal_res = cli.Get("/api/balance?userId=" + std::to_string(userId), headers);
      if (!bal_res || bal_res->status != 200) {
        walletBreaker.recordFailure();
        throw std::runtime_error("Wallet Service unreachable");
      }
      walletBreaker.recordSuccess();

      auto bal_data = json::parse(bal_res->body);
      double currentBalance = (currency == "sweep") ? bal_data["balance_sweep"].get<double>() : bal_data["balance_gold"].get<double>();

      if (currentBalance < betAmount) {
        res.status = 400;
        res.set_content(R"({"error":"Insufficient funds"})", "application/json");
        return;
      }

      // 2. Deal Cards
      Deck deck;
      deck.shuffle();
      
      std::vector<Card> playerCards = {deck.draw(), deck.draw()};
      std::vector<Card> dealerCards = {deck.draw(), deck.draw()};

      int playerScore = BlackjackEngine::calculateScore(playerCards);
      int dealerScore = BlackjackEngine::calculateScore(dealerCards);

      responseJson["playerCards"] = json::array();
      for (const auto& c : playerCards) {
        responseJson["playerCards"].push_back({{"suit", c.suit}, {"value", c.value}});
      }
      
      responseJson["dealerCards"] = json::array();
      for (const auto& c : dealerCards) {
        responseJson["dealerCards"].push_back({{"suit", c.suit}, {"value", c.value}});
      }

      responseJson["playerScore"] = playerScore;
      responseJson["dealerScore"] = dealerScore;
      responseJson["betAmount"] = betAmount;
      responseJson["currency"] = currency;

      // Check for Natural Blackjack
      if (playerScore == 21) {
        // Automatically stand and settle
        std::string result = "blackjack"; // Player has 21
        double wonAmount = 0;
        
        if (dealerScore == 21) {
          result = "push";
          wonAmount = betAmount; // Return bet
        } else {
          wonAmount = betAmount * 2.5; // Blackjack pays 3:2 (return 1 + payout 1.5 = 2.5)
        }

        // Commit transaction via Circuit Breaker
        if (!walletBreaker.allowRequest()) {
          res.status = 503;
          res.set_content(json({{"error", "Wallet Service temporarily unavailable (Circuit Breaker OPEN)"}}).dump(), "application/json");
          return;
        }

        json tx;
        tx["userId"] = userId;
        tx["currency"] = currency;
        
        if (result == "blackjack") {
          tx["amount"] = wonAmount - betAmount;
          tx["type"] = "WIN_Blackjack";
        } else {
          tx["amount"] = 0; // Return bet, net transaction amount is 0
          tx["type"] = "PUSH_Blackjack";
        }

        auto tx_res = cli.Post("/api/internal/transaction", headers, tx.dump(), "application/json");
        if (!tx_res || tx_res->status != 200) {
          walletBreaker.recordFailure();
          throw std::runtime_error("Transaction failed");
        }
        walletBreaker.recordSuccess();
        auto new_bal = json::parse(tx_res->body);

        responseJson["status"] = result;
        responseJson["wonAmount"] = wonAmount;
        responseJson["new_balance_gold"] = new_bal["balance_gold"];
        responseJson["new_balance_sweep"] = new_bal["balance_sweep"];
      } else {
        responseJson["status"] = "active";
        responseJson["wonAmount"] = 0;
      }

      res.status = 200;
      res.set_content(responseJson.dump(), "application/json");
    } catch (const std::exception &e) {
      logger->error("Exception in /blackjack/deal: " + std::string(e.what()));
      res.status = 400;
      res.set_content(json({{"error", e.what()}}).dump(), "application/json");
    }
  });

  svr.Post("/api/blackjack/hit", [&](const httplib::Request &req, httplib::Response &res) {
    setupCors(res);
    json responseJson;
    try {
      auto body = json::parse(req.body);
      int userId = body["userId"];
      double betAmount = body["betAmount"];
      std::string currency = body["currency"];
      
      std::vector<Card> playerCards;
      for (const auto& c : body["playerCards"]) {
        playerCards.emplace_back(c["suit"].get<std::string>(), c["value"].get<std::string>());
      }
      
      std::vector<Card> dealerCards;
      for (const auto& c : body["dealerCards"]) {
        dealerCards.emplace_back(c["suit"].get<std::string>(), c["value"].get<std::string>());
      }

      // Draw a card
      Deck deck;
      deck.shuffle();
      
      // Remove already used cards from the deck simulation
      // (For stateless simple deck we just draw a new card)
      Card newCard = deck.draw();
      playerCards.push_back(newCard);

      int playerScore = BlackjackEngine::calculateScore(playerCards);
      int dealerScore = BlackjackEngine::calculateScore(dealerCards);

      responseJson["playerCards"] = json::array();
      for (const auto& c : playerCards) {
        responseJson["playerCards"].push_back({{"suit", c.suit}, {"value", c.value}});
      }
      responseJson["dealerCards"] = json::array();
      for (const auto& c : dealerCards) {
        responseJson["dealerCards"].push_back({{"suit", c.suit}, {"value", c.value}});
      }

      responseJson["playerScore"] = playerScore;
      responseJson["dealerScore"] = dealerScore;
      responseJson["betAmount"] = betAmount;
      responseJson["currency"] = currency;

      if (playerScore > 21) {
        // Player busted, lose bet via Circuit Breaker
        if (!walletBreaker.allowRequest()) {
          res.status = 503;
          res.set_content(json({{"error", "Wallet Service temporarily unavailable (Circuit Breaker OPEN)"}}).dump(), "application/json");
          return;
        }

        const char *wallet_host = std::getenv("WALLET_HOST");
        std::string whost = wallet_host ? wallet_host : "casino-wallet.fly.dev";
        std::string url = whost;
        if (url.find("://") == std::string::npos) {
          url = "https://" + url;
        }
        httplib::Client cli(url);
        cli.set_connection_timeout(10, 0);
        cli.enable_server_certificate_verification(false);

        httplib::Headers headers = {
          {"X-Internal-Key", getInternalApiKey()}
        };

        json tx;
        tx["userId"] = userId;
        tx["currency"] = currency;
        tx["amount"] = betAmount;
        tx["type"] = "LOSE_Blackjack";

        auto tx_res = cli.Post("/api/internal/transaction", headers, tx.dump(), "application/json");
        if (!tx_res || tx_res->status != 200) {
          walletBreaker.recordFailure();
          throw std::runtime_error("Transaction failed");
        }
        walletBreaker.recordSuccess();
        auto new_bal = json::parse(tx_res->body);

        responseJson["status"] = "lose";
        responseJson["wonAmount"] = 0;
        responseJson["new_balance_gold"] = new_bal["balance_gold"];
        responseJson["new_balance_sweep"] = new_bal["balance_sweep"];
      } else {
        responseJson["status"] = "active";
        responseJson["wonAmount"] = 0;
      }

      res.status = 200;
      res.set_content(responseJson.dump(), "application/json");
    } catch (const std::exception &e) {
      logger->error("Exception in /blackjack/hit: " + std::string(e.what()));
      res.status = 400;
      res.set_content(json({{"error", e.what()}}).dump(), "application/json");
    }
  });

  svr.Post("/api/blackjack/stand", [&](const httplib::Request &req, httplib::Response &res) {
    setupCors(res);
    json responseJson;
    try {
      auto body = json::parse(req.body);
      int userId = body["userId"];
      double betAmount = body["betAmount"];
      std::string currency = body["currency"];
      
      std::vector<Card> playerCards;
      for (const auto& c : body["playerCards"]) {
        playerCards.emplace_back(c["suit"].get<std::string>(), c["value"].get<std::string>());
      }
      
      std::vector<Card> dealerCards;
      for (const auto& c : body["dealerCards"]) {
        dealerCards.emplace_back(c["suit"].get<std::string>(), c["value"].get<std::string>());
      }

      int playerScore = BlackjackEngine::calculateScore(playerCards);
      int dealerScore = BlackjackEngine::calculateScore(dealerCards);

      // Dealer plays rules: hit until 17 or higher
      Deck deck;
      deck.shuffle();
      while (dealerScore < 17) {
        dealerCards.push_back(deck.draw());
        dealerScore = BlackjackEngine::calculateScore(dealerCards);
      }

      std::string result = BlackjackEngine::determineResult(playerScore, dealerScore);
      double wonAmount = 0;

      if (result == "win") {
        wonAmount = betAmount * 2.0; // Payout 1:1
      } else if (result == "push") {
        wonAmount = betAmount; // Return bet
      }

      // Settle balances via Circuit Breaker
      if (!walletBreaker.allowRequest()) {
        res.status = 503;
        res.set_content(json({{"error", "Wallet Service temporarily unavailable (Circuit Breaker OPEN)"}}).dump(), "application/json");
        return;
      }

      const char *wallet_host = std::getenv("WALLET_HOST");
      std::string whost = wallet_host ? wallet_host : "casino-wallet.fly.dev";
      std::string url = whost;
      if (url.find("://") == std::string::npos) {
        url = "https://" + url;
      }
      httplib::Client cli(url);
      cli.set_connection_timeout(10, 0);
      cli.enable_server_certificate_verification(false);

      httplib::Headers headers = {
        {"X-Internal-Key", getInternalApiKey()}
      };

      json tx;
      tx["userId"] = userId;
      tx["currency"] = currency;

      if (result == "win") {
        tx["amount"] = wonAmount - betAmount;
        tx["type"] = "WIN_Blackjack";
      } else if (result == "lose") {
        tx["amount"] = betAmount;
        tx["type"] = "LOSE_Blackjack";
      } else {
        tx["amount"] = 0; // Net 0 for push
        tx["type"] = "PUSH_Blackjack";
      }

      auto tx_res = cli.Post("/api/internal/transaction", headers, tx.dump(), "application/json");
      if (!tx_res || tx_res->status != 200) {
        walletBreaker.recordFailure();
        throw std::runtime_error("Transaction failed");
      }
      walletBreaker.recordSuccess();
      auto new_bal = json::parse(tx_res->body);

      responseJson["playerCards"] = json::array();
      for (const auto& c : playerCards) {
        responseJson["playerCards"].push_back({{"suit", c.suit}, {"value", c.value}});
      }
      responseJson["dealerCards"] = json::array();
      for (const auto& c : dealerCards) {
        responseJson["dealerCards"].push_back({{"suit", c.suit}, {"value", c.value}});
      }

      responseJson["playerScore"] = playerScore;
      responseJson["dealerScore"] = dealerScore;
      responseJson["betAmount"] = betAmount;
      responseJson["currency"] = currency;
      responseJson["status"] = result;
      responseJson["wonAmount"] = wonAmount;
      responseJson["new_balance_gold"] = new_bal["balance_gold"];
      responseJson["new_balance_sweep"] = new_bal["balance_sweep"];

      res.status = 200;
      res.set_content(responseJson.dump(), "application/json");
    } catch (const std::exception &e) {
      logger->error("Exception in /blackjack/stand: " + std::string(e.what()));
      res.status = 400;
      res.set_content(json({{"error", e.what()}}).dump(), "application/json");
    }
  });

  svr.Post("/api/spin", [&](const httplib::Request &req,
                            httplib::Response &res) {

    setupCors(res);
    json responseJson;
    try {
      auto body = json::parse(req.body);
      int userId = body["userId"];
      std::string gameType = body["gameType"];
      std::string currency = body.contains("currency")
                                 ? body["currency"].get<std::string>()
                                 : "gold";

      if (gameType == "Roulette") {
        double totalBetAmount = 0;
        AdvancedRouletteEngine engine;

        if (body.contains("bets") && body["bets"].is_array()) {
          for (auto &b : body["bets"]) {
            std::string type = b["type"];
            std::string value = b["value"];
            double amount = b["amount"];
            totalBetAmount += amount;
            engine.addBet(BetFactory::createBet(type, value, amount));
          }
        } else {
          throw std::invalid_argument("Roulette requires a 'bets' array");
        }

        if (totalBetAmount <= 0)
          throw std::invalid_argument("Total bet amount must be > 0");

        // 1. COMMUNICATE WITH WALLET SERVICE TO GET BALANCE via Circuit Breaker
        if (!walletBreaker.allowRequest()) {
          res.status = 503;
          res.set_content(json({{"error", "Wallet Service temporarily unavailable (Circuit Breaker OPEN)"}}).dump(), "application/json");
          return;
        }

        const char *wallet_host = std::getenv("WALLET_HOST");
        std::string whost = wallet_host ? wallet_host : "casino-wallet.fly.dev";
        std::string url = whost;
        if (url.find("://") == std::string::npos) {
          url = "https://" + url;
        }
        httplib::Client cli(url);
        cli.set_connection_timeout(10, 0);
        cli.enable_server_certificate_verification(false);

        httplib::Headers headers = {
          {"X-Internal-Key", getInternalApiKey()}
        };

        auto bal_res = cli.Get("/api/balance?userId=" + std::to_string(userId), headers);
        if (!bal_res || bal_res->status != 200) {
          walletBreaker.recordFailure();
          throw std::runtime_error("Wallet Service is unreachable or failed");
        }
        walletBreaker.recordSuccess();

        auto bal_data = json::parse(bal_res->body);
        double currentBalance = (currency == "sweep")
                                    ? bal_data["balance_sweep"].get<double>()
                                    : bal_data["balance_gold"].get<double>();

        if (currentBalance < totalBetAmount) {
          res.status = 400;
          res.set_content(R"({"error":"Insufficient funds"})",
                          "application/json");
          return;
        }

        // 2. PLAY THE GAME
        auto [winningNumber, totalWon] = engine.spin();

        // 3. SEND TRANSACTION TO WALLET SERVICE
        json tx;
        tx["userId"] = userId;
        tx["currency"] = currency;

        if (totalWon > 0) {
          tx["amount"] = totalWon - totalBetAmount;
          tx["type"] = "WIN_Roulette";
        } else {
          tx["amount"] = totalBetAmount;
          tx["type"] = "LOSE_Roulette";
        }

        // Commit transaction via Circuit Breaker
        if (!walletBreaker.allowRequest()) {
          res.status = 503;
          res.set_content(json({{"error", "Wallet Service temporarily unavailable (Circuit Breaker OPEN)"}}).dump(), "application/json");
          return;
        }

        auto tx_res = cli.Post("/api/internal/transaction", headers, tx.dump(),
                               "application/json");
        if (!tx_res || tx_res->status != 200) {
          walletBreaker.recordFailure();
          throw std::runtime_error(
              "Failed to commit transaction to Wallet Service");
        }
        walletBreaker.recordSuccess();

        auto new_bal_data = json::parse(tx_res->body);

        responseJson["status"] = (totalWon > 0) ? "win" : "lose";
        responseJson["game"] = "Roulette";
        responseJson["winning_number"] = winningNumber;
        responseJson["won_amount"] = totalWon;
        responseJson["new_balance_gold"] = new_bal_data["balance_gold"];
        responseJson["new_balance_sweep"] = new_bal_data["balance_sweep"];

        logger->info("GameService: User " + std::to_string(userId) +
                     " played Roulette. Won: " + std::to_string(totalWon));
        res.status = 200;
        res.set_content(responseJson.dump(), "application/json");
        return;
      }

      throw std::invalid_argument(
          "Only Roulette is supported in Microservice mode");

    } catch (const std::exception &e) {
      logger->error("Exception in /api/spin: " + std::string(e.what()));
      responseJson["error"] = e.what();
      res.status = 400;
      res.set_content(responseJson.dump(), "application/json");
    }
  });

  logger->info("Game Service starting on port 8082...");
  svr.listen("0.0.0.0", 8082);
}
