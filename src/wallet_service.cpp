#include "DesignPatterns.hpp"
#include "IoCContainer.hpp"
#include "httplib.h"
#include <cstdlib>
#include <mutex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
  IoCContainer container;
  auto logger = container.getLogger();
  auto db = container.getDatabase();

  httplib::Server svr;
  std::mutex dbMutex;

  auto setupCors = [](httplib::Response &res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
  };

  svr.Options("/(.*)", [&](const httplib::Request &, httplib::Response &res) {
    setupCors(res);
    res.status = 204;
  });

  svr.Post("/api/register", [&](const httplib::Request &req, httplib::Response &res) {
    std::lock_guard<std::mutex> lock(dbMutex);
    setupCors(res);
    json responseJson;
    try {
      auto body = json::parse(req.body);
      int userId = db->registerUser(body["username"], body["password"]);
      if (userId == -1) {
        responseJson["error"] = "Користувач з таким логіном вже існує або сталася помилка.";
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

  svr.Post("/api/login", [&](const httplib::Request &req, httplib::Response &res) {
    std::lock_guard<std::mutex> lock(dbMutex);
    setupCors(res);
    json responseJson;
    try {
      auto body = json::parse(req.body);
      int userId = db->authenticateUser(body["username"], body["password"]);
      if (userId == -1) {
        responseJson["error"] = "Невірний логін або пароль.";
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

  svr.Post("/api/social_login", [&](const httplib::Request &req, httplib::Response &res) {
    std::lock_guard<std::mutex> lock(dbMutex);
    setupCors(res);
    json responseJson;
    try {
      auto body = json::parse(req.body);
      int userId = db->socialLoginUser(body["email"], body["provider"]);
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

  svr.Get("/api/balance", [&](const httplib::Request &req, httplib::Response &res) {
    std::lock_guard<std::mutex> lock(dbMutex);
    setupCors(res);
    if (!req.has_param("userId")) {
      res.status = 400;
      res.set_content(R"({"error": "Missing userId"})", "application/json");
      return;
    }
    int userId = std::stoi(req.get_param_value("userId"));
    auto balances = db->getBalances(userId);
    json j = {{"balance_gold", balances.first}, {"balance_sweep", balances.second}};
    res.set_content(j.dump(), "application/json");
  });

  // INTERNAL API FOR GAME SERVICE
  svr.Post("/api/internal/transaction", [&](const httplib::Request &req, httplib::Response &res) {
    std::lock_guard<std::mutex> lock(dbMutex);
    try {
      auto body = json::parse(req.body);
      int userId = body["userId"];
      double amount = body["amount"];
      std::string type = body["type"];
      std::string currency = body["currency"];
      
      db->saveTransaction(userId, amount, type, currency);
      auto balances = db->getBalances(userId);
      json response = {
          {"status", "success"},
          {"balance_gold", balances.first},
          {"balance_sweep", balances.second}
      };
      res.set_content(response.dump(), "application/json");
    } catch (const std::exception &e) {
      logger->error("Internal Transaction Error: " + std::string(e.what()));
      res.status = 400;
      res.set_content(R"({"status":"error"})", "application/json");
    }
  });

  logger->info("Wallet Service starting on port 8081...");
  svr.listen("0.0.0.0", 8081);
}
