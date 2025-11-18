cmake_minimum_required(VERSION 3.14)
project(practice_emulator)

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
        tests/tests1.cpp include/emu.h src/emu.cpp include/cache.h src/cache.cpp
)

add_executable(
        emu
        include/emu.h src/emu.cpp src/main.cpp include/cache.h src/cache.cpp
)

target_link_libraries(
        runTests
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(runTests)
