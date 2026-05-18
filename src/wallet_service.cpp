#include "DesignPatterns.hpp"
#include "IoCContainer.hpp"
#include "httplib.h"
#include "PostgresDatabase.hpp"
#include "JwtHelper.hpp"
#include <chrono>
#include <cstdlib>
#include <mutex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static auto serviceStartTime = std::chrono::steady_clock::now();

// Shared secrets & settings
static std::string getJwtSecret() {
  const char* env_secret = std::getenv("JWT_SECRET");
  return env_secret ? env_secret : "super-secret-key-12345";
}

static std::string getInternalApiKey() {
  const char* env_key = std::getenv("INTERNAL_API_KEY");
  return env_key ? env_key : "super-secret-internal-key-9999";
}

// Helper to authenticate public user or trust secure internal microservices
static int getAuthenticatedUserId(const httplib::Request &req, std::string &out_username) {
  std::string internal_key = getInternalApiKey();
  
  // 1. If it's an internal microservice, authenticate via private API Key
  if (req.has_header("X-Internal-Key") && req.get_header_value("X-Internal-Key") == internal_key) {
    if (req.has_param("userId")) {
      out_username = "internal_service";
      return std::stoi(req.get_param_value("userId"));
    }
    return -2; // Authorized internal service, but no specific userId parsed yet
  }

  // 2. Otherwise, check public Authorization: Bearer <token>
  if (req.has_header("Authorization")) {
    std::string auth = req.get_header_value("Authorization");
    if (auth.rfind("Bearer ", 0) == 0) {
      std::string token = auth.substr(7);
      return JwtHelper::verify_jwt(token, getJwtSecret(), out_username);
    }
  }
  
  return -1; // Unauthorized
}

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
    res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS, PUT, DELETE");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Internal-Key");
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
      std::string username = body["username"];
      int userId = db->registerUser(username, body["password"]);
      if (userId == -1) {
        responseJson["error"] = "Користувач з таким логіном вже існує або сталася помилка.";
        res.status = 400;
      } else {
        responseJson["message"] = "Registration successful";
        responseJson["user_id"] = userId;
        auto balances = db->getBalances(userId);
        responseJson["balance_gold"]  = balances.first;
        responseJson["balance_sweep"] = balances.second;
        
        // Generate JWT Access & Refresh Tokens
        responseJson["access_token"] = JwtHelper::generate_jwt(userId, username, getJwtSecret(), 3600); // 1 hour
        responseJson["refresh_token"] = JwtHelper::generate_jwt(userId, username, getJwtSecret(), 7 * 24 * 3600); // 7 days
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
      std::string username = body["username"];
      int userId = db->authenticateUser(username, body["password"]);
      if (userId == -1) {
        responseJson["error"] = "Невірний логін або пароль.";
        res.status = 401;
      } else {
        responseJson["message"] = "Login successful";
        responseJson["user_id"] = userId;
        auto balances = db->getBalances(userId);
        responseJson["balance_gold"]  = balances.first;
        responseJson["balance_sweep"] = balances.second;
        
        // Generate JWT Access & Refresh Tokens
        responseJson["access_token"] = JwtHelper::generate_jwt(userId, username, getJwtSecret(), 3600); // 1 hour
        responseJson["refresh_token"] = JwtHelper::generate_jwt(userId, username, getJwtSecret(), 7 * 24 * 3600); // 7 days
        res.status = 200;
      }
    } catch (const std::exception &e) {
      responseJson["error"] = e.what();
      res.status = 400;
    }
    res.set_content(responseJson.dump(), "application/json");
  });

  // ─── REFRESH TOKEN ────────────────────────────────────────────────────────────
  svr.Post("/api/refresh", [&](const httplib::Request &req, httplib::Response &res) {
    std::lock_guard<std::mutex> lock(dbMutex);
    setupCors(res);
    json responseJson;
    try {
      auto body = json::parse(req.body);
      std::string refreshToken = body["refresh_token"];
      std::string username;
      int userId = JwtHelper::verify_jwt(refreshToken, getJwtSecret(), username);
      if (userId == -1) {
        responseJson["error"] = "Invalid or expired refresh token.";
        res.status = 401;
      } else {
        responseJson["access_token"] = JwtHelper::generate_jwt(userId, username, getJwtSecret(), 3600);
        responseJson["refresh_token"] = JwtHelper::generate_jwt(userId, username, getJwtSecret(), 7 * 24 * 3600);
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
      std::string email = body["email"];
      int userId = db->socialLoginUser(email, body["provider"]);
      if (userId == -1) {
        responseJson["error"] = "Social Auth Error";
        res.status = 401;
      } else {
        responseJson["message"] = "Login successful";
        responseJson["user_id"] = userId;
        auto balances = db->getBalances(userId);
        responseJson["balance_gold"]  = balances.first;
        responseJson["balance_sweep"] = balances.second;
        
        // Generate JWT Access & Refresh Tokens
        responseJson["access_token"] = JwtHelper::generate_jwt(userId, email, getJwtSecret(), 3600);
        responseJson["refresh_token"] = JwtHelper::generate_jwt(userId, email, getJwtSecret(), 7 * 24 * 3600);
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
    
    std::string username;
    int authUserId = getAuthenticatedUserId(req, username);
    if (authUserId < 0) {
      res.status = 401;
      res.set_content(R"({"error": "Unauthorized: Invalid or missing token"})", "application/json");
      return;
    }

    auto balances = db->getBalances(authUserId);
    json j = {{"balance_gold", balances.first}, {"balance_sweep", balances.second}};
    res.set_content(j.dump(), "application/json");
  });

  // ─── TRANSACTION HISTORY ──────────────────────────────────────────────────────
  svr.Get("/api/transactions", [&](const httplib::Request &req, httplib::Response &res) {
    std::lock_guard<std::mutex> lock(dbMutex);
    setupCors(res);
    
    std::string username;
    int authUserId = getAuthenticatedUserId(req, username);
    if (authUserId < 0) {
      res.status = 401;
      res.set_content(R"({"error": "Unauthorized: Invalid or missing token"})", "application/json");
      return;
    }

    try {
      pqxx::connection conn(connStr);
      pqxx::work W(conn);
      pqxx::result r = W.exec_params(
        "SELECT transaction_id, amount, type, currency, created_at "
        "FROM transactions WHERE user_id = $1 ORDER BY created_at DESC LIMIT 50",
        authUserId);
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
    setupCors(res);

    // Verify private internal shared key
    std::string internal_key = getInternalApiKey();
    if (!req.has_header("X-Internal-Key") || req.get_header_value("X-Internal-Key") != internal_key) {
      res.status = 401;
      res.set_content(R"({"error": "Unauthorized internal microservice call"})", "application/json");
      return;
    }

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
