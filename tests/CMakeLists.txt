file(GLOB TEST_SOURCES "*.cpp")
add_executable(casino_tests ${TEST_SOURCES})

target_link_libraries(casino_tests
    PRIVATE
    gtest_main
    casino_core
)

include(GoogleTest)
gtest_discover_tests(casino_tests)
