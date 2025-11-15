cmake_minimum_required(VERSION 3.14)
project(Project-3)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL
        https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
        runTests
        tests/tests1.cpp include/emu4380.h src/emu4380.cpp include/cache.h src/cache.cpp
)

add_executable(
        emu4380
        include/emu4380.h src/emu4380.cpp src/main.cpp include/cache.h src/cache.cpp
)

target_link_libraries(
        runTests
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(runTests)