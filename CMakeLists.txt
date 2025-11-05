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

# Add an option to enable/disable tests. toggle with -DBUILD_TESTING=ON/OFF
option(BUILD_TESTING "Build the testing suite" ON)

if(BUILD_TESTING)
  enable_testing()

  include(FetchContent)
  FetchContent_Declare(
    googletest URL https://github.com/google/googletest/archive/v1.14.0.zip)
  FetchContent_MakeAvailable(googletest)

  add_executable(tests src/tests/test.cpp)

  target_link_libraries(tests PRIVATE GTest::gtest_main ${PROJECT_NAME}_lib)

  # Include the GoogleTest module to discover tests automatically.
  include(GoogleTest)
  gtest_discover_tests(tests)
endif()
