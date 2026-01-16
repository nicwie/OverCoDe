#include "ArgsParser.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

// Helper to convert C++ strings to char* array for argv
std::vector<char *> makeArgv(const std::vector<std::string> &args) {
  std::vector<char *> argv;
  for (const auto &arg : args) {
    argv.push_back(const_cast<char *>(arg.data()));
  }
  return argv;
}

TEST(ArgsParserTest, FixInitializationOrderBug) {
  // Usage: ./main false Alpha Beta OutputFile Graphs Runs overlapSize
  std::vector<std::string> args = {
      "./OverCoDe", 
      "false",
      "0.92",         // alpha
      "0.95",         // beta
      "result.txt",
      "20",           // Graphs
      "20",           // Runs
      "112",
      "37",
      "12"            // Overlaps
  };

  std::vector<char *> argv = makeArgv(args);
  int argc = static_cast<int>(argv.size());

  AppParams params = parseArgs(argc, argv.data());

  // n should be 5000 for false case
  EXPECT_EQ(params.n, 5000);

  // T should be positive. log2(5000) ~ 12.28. T = 10 * 12.28 = 122.
  // If bug exists, T would be negative/garbage.
  EXPECT_GT(params.T, 0);
  EXPECT_EQ(params.T, 122);
  EXPECT_GT(params.l, 0);
  EXPECT_GT(params.k, 0);
  EXPECT_GT(params.h, 0);
}

TEST(ArgsParserTest, EgoGraphParams) {
  // Usage: ./main true alpha beta OutputFile Graphs Runs
  std::vector<std::string> args = {"./OverCoDe", "true", "0.92", "0.95",
                                   "result.txt", "1",    "1"};

  std::vector<char *> argv = makeArgv(args);
  int argc = static_cast<int>(argv.size());

  AppParams params = parseArgs(argc, argv.data());

  EXPECT_EQ(params.n, 125);
  EXPECT_GT(params.T, 0);
}
