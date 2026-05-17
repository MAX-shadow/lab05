cmake_minimum_required(VERSION 3.10)
project(lab05)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build tests" OFF)
option(COVERAGE "Enable coverage instrumentation" OFF)

if(COVERAGE)
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

add_subdirectory(banking)

if(BUILD_TESTS)
    enable_testing()
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    add_subdirectory(third-party/gtest)

    add_executable(check tests/test_account.cpp tests/test_transaction.cpp)
    target_link_libraries(check banking gtest_main gmock)
    target_include_directories(check PRIVATE banking)

    add_test(NAME check COMMAND check)
endif()
