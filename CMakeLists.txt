cmake_minimum_required(VERSION 3.15)

cmake_policy(SET CMP0135 NEW)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Project name, version, and language.
project(
  OverCoDe
  VERSION 1.0
  LANGUAGES CXX)

# C++ Standard Configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_COVERAGE "Enable instrumentation for code coverage" OFF)

if(ENABLE_COVERAGE)
  if("${CMAKE_CXX_COMPILER_ID}" MATCHES "(GNU|Clang)")
    # --coverage adds -fprofile-arcs and -ftest-coverage
    add_compile_options(--coverage)
    add_link_options(--coverage)
    message(STATUS "Code coverage instrumentation enabled")
  else()
    message(WARNING "Code coverage usually requires GCC or Clang.")
  endif()
endif()

set(GCC_CLANG_WARNING_FLAGS
    -Wall
    -Wextra
    -Wpedantic
    -Wuninitialized
    -Wshadow
    -Wconversion
    -Werror)

add_executable(${PROJECT_NAME} src/main.cpp)

target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_options(
  ${PROJECT_NAME}
  PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:${GCC_CLANG_WARNING_FLAGS}>)
target_compile_options(
  ${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:$<$<CXX_COMPILER_ID:GNU,Clang>:-g>>)

target_compile_options(
  ${PROJECT_NAME}
  PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:${GCC_CLANG_WARNING_FLAGS}>)

find_package(Threads REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

# Option to enable/disable tests. toggle with -DBUILD_TESTING=ON/OFF
option(BUILD_TESTING "Build the testing suite" ON)

if(BUILD_TESTING)
  enable_testing()

  include(FetchContent)
  FetchContent_Declare(
    googletest URL https://github.com/google/googletest/archive/v1.14.0.zip)
  FetchContent_MakeAvailable(googletest)

  add_executable(
    tests
    tests/test_CLUSTEREDGRAPH.cpp tests/test_SYNTHETICEGOGRAPH.cpp
    tests/test_GRAPH.cpp tests/test_OVERCODE.cpp tests/test_RANDOMGENERATOR.cpp)

  target_include_directories(tests
                             PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

  target_link_libraries(tests PRIVATE GTest::gtest_main)

  # Include the GoogleTest module to discover tests automatically.
  include(GoogleTest)
  gtest_discover_tests(tests DISCOVERY_MODE PRE_TEST)
endif()
