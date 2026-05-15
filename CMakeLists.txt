cmake_minimum_required(VERSION 3.14)
project(CasinoPlatform CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Fetch cpp-httplib (REST API) та nlohmann_json (для роботи з JSON)
FetchContent_Declare(
  httplib
  URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.18.3.zip
)
FetchContent_MakeAvailable(httplib)

FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

add_library(casino_core src/Wallet.cpp)
target_include_directories(casino_core PUBLIC include)

# Основний виконуваний файл бекенду (API)
add_executable(casino_server src/main.cpp)

# Підключаємо libpqxx та libpq напряму через шляхи Homebrew (macOS ARM64)
if(APPLE)
    target_include_directories(casino_server PRIVATE /opt/homebrew/include /opt/homebrew/opt/libpq/include)
    target_link_directories(casino_server PRIVATE /opt/homebrew/lib /opt/homebrew/opt/libpq/lib)
endif()
target_link_libraries(casino_server PRIVATE casino_core httplib::httplib nlohmann_json::nlohmann_json pqxx pq)

add_subdirectory(tests)
