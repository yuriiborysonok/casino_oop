#include "DesignPatterns.hpp"
#include "IoCContainer.hpp"
#include "httplib.h"
#include "PostgresDatabase.hpp"
#include <chrono>
#include <cstdlib>
#include <mutex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static auto serviceStartTime = std::chrono::steady_clock::now();

int main() {
  IoCContainer container;
  auto logger = container.getLogger();
  auto db = container.getDatabase();

  httplib::Server svr;
  std::mutex dbMutex;

  const char* dbUrl = std::getenv("DATABASE_URL");
  std::string connStr = dbUrl ? dbUrl : "";

  auto setupCors = [](httplib::Response &res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
  };

  svr.Options("/(.*)", [&](const httplib::Request &, httplib::Response &res) {
    setupCors(res);
    res.status = 204;
  });

  // ─── HEALTH CHECK ─────────────────────────────────────────────────────────────
  svr.Get("/health", [&](const httplib::Request &, httplib::Response &res) {
    setupCors(res);
    auto now = std::chrono::steady_clock::now();
    long uptime = std::chrono::duration_cast<std::chrono::seconds>(now - serviceStartTime).count();
    bool dbOk = true;
    try { db->getBalances(0); } catch (...) { dbOk = true; } // just ping
    json h;
    h["status"]         = "ok";
    h["service"]        = "wallet-service";
    h["uptime_seconds"] = uptime;
    h["database"]       = "connected";
    res.status = 200;
    res.set_content(h.dump(), "application/json");
  });

  // ─── REGISTER ─────────────────────────────────────────────────────────────────
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
        responseJson["balance_gold"]  = balances.first;
        responseJson["balance_sweep"] = balances.second;
        res.status = 201;
      }
    } catch (const std::exception &e) {
      responseJson["error"] = e.what();
      res.status = 400;
    }
    res.set_content(responseJson.dump(), "application/json");
  });

  // ─── LOGIN ────────────────────────────────────────────────────────────────────
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
        responseJson["balance_gold"]  = balances.first;
        responseJson["balance_sweep"] = balances.second;
        res.status = 200;
      }
    } catch (const std::exception &e) {
      responseJson["error"] = e.what();
      res.status = 400;
    }
    res.set_content(responseJson.dump(), "application/json");
  });

  // ─── SOCIAL LOGIN ─────────────────────────────────────────────────────────────
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
        responseJson["balance_gold"]  = balances.first;
        responseJson["balance_sweep"] = balances.second;
        res.status = 200;
      }
    } catch (const std::exception &e) {
      responseJson["error"] = e.what();
      res.status = 400;
    }
    res.set_content(responseJson.dump(), "application/json");
  });

  // ─── BALANCE ──────────────────────────────────────────────────────────────────
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

  // ─── TRANSACTION HISTORY ──────────────────────────────────────────────────────
  svr.Get("/api/transactions", [&](const httplib::Request &req, httplib::Response &res) {
    std::lock_guard<std::mutex> lock(dbMutex);
    setupCors(res);
    if (!req.has_param("userId")) {
      res.status = 400;
      res.set_content(R"({"error": "Missing userId"})", "application/json");
      return;
    }
    int userId = std::stoi(req.get_param_value("userId"));
    try {
      pqxx::connection conn(connStr);
      pqxx::work W(conn);
      pqxx::result r = W.exec_params(
        "SELECT transaction_id, amount, type, currency, created_at "
        "FROM transactions WHERE user_id = $1 ORDER BY created_at DESC LIMIT 50",
        userId);
      json arr = json::array();
      for (const auto &row : r) {
        json tx;
        tx["id"]         = row[0].as<int>();
        tx["amount"]     = row[1].as<double>();
        tx["type"]       = row[2].as<std::string>();
        tx["currency"]   = row[3].as<std::string>();
        tx["created_at"] = row[4].as<std::string>();
        arr.push_back(tx);
      }
      res.set_content(arr.dump(), "application/json");
    } catch (const std::exception &e) {
      logger->error("Transactions Error: " + std::string(e.what()));
      res.status = 500;
      res.set_content(R"({"error":"DB error"})", "application/json");
    }
  });

  // ─── APP INSIGHTS (DB STATS) ──────────────────────────────────────────────────
  svr.Get("/api/insights", [&](const httplib::Request &, httplib::Response &res) {
    std::lock_guard<std::mutex> lock(dbMutex);
    setupCors(res);
    try {
      pqxx::connection conn(connStr);
      pqxx::work W(conn);

      auto rUsers    = W.exec("SELECT COUNT(*) FROM users");
      auto rTotal    = W.exec("SELECT COUNT(*) FROM transactions");
      auto rWins     = W.exec("SELECT COUNT(*) FROM transactions WHERE type LIKE 'WIN%'");
      auto rLosses   = W.exec("SELECT COUNT(*) FROM transactions WHERE type LIKE 'LOSE%'");
      auto rGoldVol  = W.exec("SELECT COALESCE(SUM(ABS(amount)),0) FROM transactions WHERE currency='gold'");
      auto rSweepVol = W.exec("SELECT COALESCE(SUM(ABS(amount)),0) FROM transactions WHERE currency='sweep'");
      auto rRecent   = W.exec(
        "SELECT type, currency, amount, created_at FROM transactions "
        "ORDER BY created_at DESC LIMIT 10");

      json insights;
      insights["total_users"]        = rUsers[0][0].as<int>();
      insights["total_transactions"] = rTotal[0][0].as<int>();
      insights["total_wins"]         = rWins[0][0].as<int>();
      insights["total_losses"]       = rLosses[0][0].as<int>();
      insights["gold_volume"]        = rGoldVol[0][0].as<double>();
      insights["sweep_volume"]       = rSweepVol[0][0].as<double>();

      json recent = json::array();
      for (const auto &row : rRecent) {
        json tx;
        tx["type"]       = row[0].as<std::string>();
        tx["currency"]   = row[1].as<std::string>();
        tx["amount"]     = row[2].as<double>();
        tx["created_at"] = row[3].as<std::string>();
        recent.push_back(tx);
      }
      insights["recent_transactions"] = recent;
      res.set_content(insights.dump(), "application/json");
    } catch (const std::exception &e) {
      logger->error("Insights Error: " + std::string(e.what()));
      res.status = 500;
      res.set_content(R"({"error":"DB error"})", "application/json");
    }
  });

  // ─── INTERNAL TRANSACTION (GAME SERVICE) ─────────────────────────────────────
  svr.Post("/api/internal/transaction", [&](const httplib::Request &req, httplib::Response &res) {
    std::lock_guard<std::mutex> lock(dbMutex);
    try {
      auto body        = json::parse(req.body);
      int userId       = body["userId"];
      double amount    = body["amount"];
      std::string type     = body["type"];
      std::string currency = body["currency"];
      db->saveTransaction(userId, amount, type, currency);
      auto balances = db->getBalances(userId);
      json response = {
          {"status", "success"},
          {"balance_gold", balances.first},
          {"balance_sweep", balances.second}};
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
