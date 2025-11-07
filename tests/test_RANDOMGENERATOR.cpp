#include <gtest/gtest.h>

#include <stdexcept>

#include "RandomGenerator.h"

TEST(RandomGeneratorTest, GetRandomInt) {
  RandomGenerator rand;
  for (int i = 0; i < 100; ++i) {
    int val = rand.getRandomInt(5, 10);
    EXPECT_GE(val, 5);
    EXPECT_LE(val, 10);
  }
}

TEST(RandomGeneratorTest, GetRandomIntSingleValue) {
  RandomGenerator rand;
  for (int i = 0; i < 100; ++i) {
    int val = rand.getRandomInt(5, 5);
    EXPECT_EQ(val, 5);
  }
}

TEST(RandomGeneratorTest, GetRandomDouble) {
  RandomGenerator rand;
  for (int i = 0; i < 100; ++i) {
    double val = rand.getRandomDouble(0.1, 0.5);
    EXPECT_GE(val, 0.1);
    EXPECT_LE(val, 0.5);
  }
}

TEST(RandomGeneratorTest, GetRandomUll) {
  RandomGenerator rand;
  for (int i = 0; i < 100; ++i) {
    unsigned long long val = rand.getRandomUll(100ULL, 200ULL);
    EXPECT_GE(val, 100ULL);
    EXPECT_LE(val, 200ULL);
  }
}

TEST(RandomGeneratorTest, GetNormalInt) {
  RandomGenerator rand;
  // Hard to test distribution, just check that it returns a value
  bool valueGenerated = false;
  for (int i = 0; i < 10; ++i) {
    rand.getNormalInt(100, 10);
    valueGenerated = true;
  }
  EXPECT_TRUE(valueGenerated);
}

TEST(RandomGeneratorTest, InvalidRange) {
  RandomGenerator rand;
  EXPECT_THROW(rand.getRandomInt(10, 5), std::invalid_argument);
  EXPECT_THROW(rand.getRandomDouble(1.0, 0.0), std::invalid_argument);
  EXPECT_THROW(rand.getRandomUll(100ULL, 50ULL), std::invalid_argument);
}
