file(GLOB TEST_SOURCES "*.cpp")
add_executable(casino_tests ${TEST_SOURCES})

if(APPLE)
    target_include_directories(casino_tests PRIVATE /opt/homebrew/include /opt/homebrew/opt/libpq/include)
    target_link_directories(casino_tests PRIVATE /opt/homebrew/lib /opt/homebrew/opt/libpq/lib)
endif()

target_link_libraries(casino_tests
    PRIVATE
    gtest_main
    casino_core
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    pqxx
    pq
)

include(GoogleTest)
gtest_discover_tests(casino_tests)
