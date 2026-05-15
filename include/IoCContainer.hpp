#pragma once
#include "IDatabase.hpp"
#include "ILogger.hpp"
#include "PostgresDatabase.hpp"
#include "SpdLogger.hpp"
#include <memory>

class IoCContainer {
private:
  std::shared_ptr<ILogger> logger;
  std::shared_ptr<IDatabase> database;

public:
  IoCContainer() {
    // контейнер автоматом збирає систему і створює логер
    logger = std::make_shared<SpdLogger>();

    // зчитуємо урл бази з змінної середовища, інакше локальний дефолт
    const char *env_db_url = std::getenv("DATABASE_URL");
    std::string dbStr = env_db_url ? std::string(env_db_url)
                                   : "dbname=casino_db user=postgres "
                                     "password=secret host=localhost port=5432";

    // інжект логера у базу даних під час її створення
    database = std::make_shared<PostgresDatabase>(logger, dbStr);
  }

  std::shared_ptr<ILogger> getLogger() { return logger; }
  std::shared_ptr<IDatabase> getDatabase() { return database; }
};
