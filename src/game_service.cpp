#include "DesignPatterns.hpp"
#include "RouletteEngine.hpp"
#include "SpdLogger.hpp"
#include "httplib.h"
#include <cstdlib>
#include <ctime>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

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

        // 1. COMMUNICATE WITH WALLET SERVICE TO GET BALANCE
        const char *wallet_host = std::getenv("WALLET_HOST");
        std::string whost = wallet_host ? wallet_host : "casino-wallet.fly.dev";
        httplib::SSLClient cli(whost);
        cli.set_connection_timeout(10, 0);
        cli.enable_server_certificate_verification(false);

        auto bal_res = cli.Get("/api/balance?userId=" + std::to_string(userId));
        if (!bal_res || bal_res->status != 200) {
          throw std::runtime_error("Wallet Service is unreachable or failed");
        }

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

        auto tx_res = cli.Post("/api/internal/transaction", tx.dump(),
                               "application/json");
        if (!tx_res || tx_res->status != 200) {
          throw std::runtime_error(
              "Failed to commit transaction to Wallet Service");
        }

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
