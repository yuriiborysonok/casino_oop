#pragma once
#include "IDatabase.hpp"
#include "ILogger.hpp"
#include "PostgresDatabase.hpp"
#include "SpdLogger.hpp"
#include <memory>

// IoC Контейнер (Inversion of Control Container)
// Відповідає за створення об'єктів та ін'єкцію їх залежностей (Wiring)
class IoCContainer {
private:
  std::shared_ptr<ILogger> logger;
  std::shared_ptr<IDatabase> database;

public:
  IoCContainer() {
    // Контейнер автоматично "збирає" систему:
    // 1. Створює логер
    logger = std::make_shared<SpdLogger>();

    // Зчитуємо URL бази з змінної середовища (для Docker/Cloud Run), інакше локальний дефолт
    const char* env_db_url = std::getenv("DATABASE_URL");
    std::string dbStr = env_db_url ? std::string(env_db_url) : "dbname=casino_db user=postgres password=secret host=localhost port=5432";

    // 2. Вставляє (Inject) логер у базу даних під час її створення
    database = std::make_shared<PostgresDatabase>(logger, dbStr);
  }

  std::shared_ptr<ILogger> getLogger() { return logger; }
  std::shared_ptr<IDatabase> getDatabase() { return database; }
};
