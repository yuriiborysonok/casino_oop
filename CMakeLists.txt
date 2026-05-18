cmake_minimum_required(VERSION 3.14)
project(CasinoPlatform CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
endif()

include_directories(include)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

find_package(OpenSSL REQUIRED)
add_compile_definitions(CPPHTTPLIB_OPENSSL_SUPPORT)

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

add_executable(wallet_service src/wallet_service.cpp)
add_executable(game_service src/game_service.cpp)

if(APPLE)
    target_include_directories(wallet_service PRIVATE /opt/homebrew/include /opt/homebrew/opt/libpq/include)
    target_link_directories(wallet_service PRIVATE /opt/homebrew/lib /opt/homebrew/opt/libpq/lib)
    target_include_directories(game_service PRIVATE /opt/homebrew/include /opt/homebrew/opt/libpq/include)
    target_link_directories(game_service PRIVATE /opt/homebrew/lib /opt/homebrew/opt/libpq/lib)
endif()

target_link_libraries(wallet_service PRIVATE casino_core httplib::httplib nlohmann_json::nlohmann_json pqxx pq OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(game_service PRIVATE casino_core httplib::httplib nlohmann_json::nlohmann_json OpenSSL::SSL OpenSSL::Crypto)

add_subdirectory(tests)
