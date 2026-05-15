#include "DesignPatterns.hpp"
#include "IoCContainer.hpp"
#include "RouletteEngine.hpp"
#include "Wallet.hpp"
#include "httplib.h"
#include <cstdlib>
#include <ctime>
#include <mutex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
  // 1. Ініціалізуємо IoC Контейнер
  IoCContainer container;
  auto logger = container.getLogger();
  auto db = container.getDatabase();

  httplib::Server svr;
  std::mutex dbMutex;

  logger->info("Starting Casino Microservice with Multi-User Support...");
  std::srand(std::time(nullptr));

  // Налаштування CORS
  auto setupCors = [](httplib::Response &res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
  };

  // Роздача статичних файлів (Frontend)
  svr.set_mount_point("/", "./frontend");

  // OPTIONS для всіх ендпоінтів
  svr.Options("/(.*)", [&](const httplib::Request &, httplib::Response &res) {
    setupCors(res);
    res.status = 204;
  });

  // Ендпоінт: Реєстрація
  svr.Post("/api/register", [&](const httplib::Request &req,
                                httplib::Response &res) {
    std::lock_guard<std::mutex> lock(dbMutex);
    setupCors(res);
    json responseJson;

    try {
      auto body = json::parse(req.body);
      std::string username = body["username"];
      std::string password = body["password"];

      int userId = db->registerUser(username, password);
      if (userId == -1) {
        responseJson["error"] =
            "Користувач з таким логіном вже існує або сталася помилка.";
        res.status = 400;
      } else {
        responseJson["message"] = "Registration successful";
        responseJson["user_id"] = userId;
        auto balances = db->getBalances(userId);
        responseJson["balance_gold"] = balances.first;
        responseJson["balance_sweep"] = balances.second;
        res.status = 201;
      }
    } catch (const std::exception &e) {
      responseJson["error"] = e.what();
      res.status = 400;
    }
    res.set_content(responseJson.dump(), "application/json");
  });

  // Ендпоінт: Логін
  svr.Post("/api/login",
           [&](const httplib::Request &req, httplib::Response &res) {
             std::lock_guard<std::mutex> lock(dbMutex);
             setupCors(res);
             json responseJson;

             try {
               auto body = json::parse(req.body);
               std::string username = body["username"];
               std::string password = body["password"];

               int userId = db->authenticateUser(username, password);
               if (userId == -1) {
                 responseJson["error"] = "Невірний логін або пароль.";
                 res.status = 401; // Unauthorized
               } else {
                 responseJson["message"] = "Login successful";
                 responseJson["user_id"] = userId;
                 auto balances = db->getBalances(userId);
                 responseJson["balance_gold"] = balances.first;
                 responseJson["balance_sweep"] = balances.second;
                 res.status = 200;
               }
             } catch (const std::exception &e) {
               responseJson["error"] = e.what();
               res.status = 400;
             }
             res.set_content(responseJson.dump(), "application/json");
           });

  // Ендпоінт: Соціальний Логін (Google/Facebook)
  svr.Post("/api/social_login",
           [&](const httplib::Request &req, httplib::Response &res) {
             std::lock_guard<std::mutex> lock(dbMutex);
             setupCors(res);
             json responseJson;

             try {
               auto body = json::parse(req.body);
               std::string email = body["email"];
               std::string provider = body["provider"];

               int userId = db->socialLoginUser(email, provider);
               if (userId == -1) {
                 responseJson["error"] = "Social Auth Error";
                 res.status = 401;
               } else {
                 responseJson["message"] = "Login successful";
                 responseJson["user_id"] = userId;
                 auto balances = db->getBalances(userId);
                 responseJson["balance_gold"] = balances.first;
                 responseJson["balance_sweep"] = balances.second;
                 res.status = 200;
               }
             } catch (const std::exception &e) {
               responseJson["error"] = e.what();
               res.status = 400;
             }
             res.set_content(responseJson.dump(), "application/json");
           });

  // Ендпоінт: Отримати баланс (GET)
  svr.Get(
      "/api/balance", [&](const httplib::Request &req, httplib::Response &res) {
        std::lock_guard<std::mutex> lock(dbMutex);
        setupCors(res);

        if (!req.has_param("userId")) {
          res.status = 400;
          res.set_content(R"({"error": "Missing userId"})", "application/json");
          return;
        }

        int userId = std::stoi(req.get_param_value("userId"));
        auto balances = db->getBalances(userId);

        json j;
        j["balance_gold"] = balances.first;
        j["balance_sweep"] = balances.second;
        res.set_content(j.dump(), "application/json");
      });

  // Ендпоінт: Зробити спін (POST)
  svr.Post("/api/spin", [&](const httplib::Request &req,
                            httplib::Response &res) {
    std::lock_guard<std::mutex> lock(dbMutex);
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
        // Advanced OOP Roulette flow
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

        Wallet playerWallet;
        auto balances = db->getBalances(userId);
        double currentBalance =
            (currency == "sweep") ? balances.second : balances.first;

        playerWallet.deposit(currentBalance);
        playerWallet.withdraw(totalBetAmount); // Deduct the bet amount

        auto [winningNumber, totalWon] = engine.spin();

        if (totalWon > 0) {
          playerWallet.deposit(totalWon);
          db->saveTransaction(userId, totalWon, "WIN_Roulette", currency);
          logger->info("User " + std::to_string(userId) + " won $" +
                       std::to_string(totalWon) + " on Roulette (Number: " +
                       std::to_string(winningNumber) + ")");
        } else {
          db->saveTransaction(userId, totalBetAmount, "LOSE_Roulette",
                              currency);
          logger->info(
              "User " + std::to_string(userId) + " lost $" +
              std::to_string(totalBetAmount) +
              " on Roulette (Number: " + std::to_string(winningNumber) + ")");
        }

        auto newBalances = db->getBalances(userId);
        responseJson["status"] = (totalWon > 0) ? "win" : "lose";
        responseJson["game"] = "Roulette";
        responseJson["winning_number"] = winningNumber;
        responseJson["won_amount"] = totalWon;
        responseJson["new_balance_gold"] = newBalances.first;
        responseJson["new_balance_sweep"] = newBalances.second;

        res.status = 200;
        res.set_content(responseJson.dump(), "application/json");
        return;
      }

      // Old fallback flow for Slots (50/50 logic)
      double betAmount =
          body.contains("betAmount") ? body["betAmount"].get<double>() : 0;
      if (betAmount <= 0)
        throw std::invalid_argument("betAmount must be > 0");

      Wallet playerWallet;
      auto balances = db->getBalances(userId);
      double currentBalance =
          (currency == "sweep") ? balances.second : balances.first;

      playerWallet.deposit(currentBalance);
      playerWallet.withdraw(betAmount);

      auto game = GameFactory::createGame(gameType);
      bool isWin = (std::rand() % 2 == 0);

      if (isWin) {
        auto baseStrategy = std::make_unique<ColorPayoutStrategy>();
        HappyHourBonusDecorator bonusStrategy(std::move(baseStrategy));

        double winAmount = bonusStrategy.calculatePayout(betAmount);
        playerWallet.deposit(winAmount);

        db->saveTransaction(userId, winAmount, "WIN_" + gameType, currency);
        logger->info("User " + std::to_string(userId) + " won $" +
                     std::to_string(winAmount) + " in " + gameType);

        responseJson["status"] = "win";
        responseJson["game"] = game->getName();
        responseJson["won_amount"] = winAmount;
      } else {
        db->saveTransaction(userId, betAmount, "LOSE_" + gameType, currency);
        logger->info("User " + std::to_string(userId) + " lost $" +
                     std::to_string(betAmount) + " in " + gameType);

        responseJson["status"] = "lose";
        responseJson["game"] = game->getName();
        responseJson["won_amount"] = 0.0;
      }

      auto newBalances = db->getBalances(userId);
      responseJson["new_balance_gold"] = newBalances.first;
      responseJson["new_balance_sweep"] = newBalances.second;
      res.status = 200;
      res.set_content(responseJson.dump(), "application/json");

    } catch (const std::exception &e) {
      logger->error("Exception in /api/spin: " + std::string(e.what()));
      responseJson["error"] = e.what();
      res.status = 400;
      res.set_content(responseJson.dump(), "application/json");
    }
  });

  const char *env_p = std::getenv("PORT");
  int port = env_p ? std::stoi(env_p) : 8080;
  logger->info("Starting Casino API server on port " + std::to_string(port) +
               "...");
  svr.listen("0.0.0.0", port);
}
