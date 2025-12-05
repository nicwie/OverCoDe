#ifndef RANDOMGENERATOR_H_INCLUDED
#define RANDOMGENERATOR_H_INCLUDED

#include <limits>
#include <random> // For mt19937 and uniform distributions
#include <stdexcept>

class RandomGenerator {
public:
  RandomGenerator() {
    std::random_device rd;
    rng = std::mt19937(rd());
  }

  // Generate a random integer between min and max (inclusive)
  int getRandomInt(const int min, const int max) {
    if (min > max) {
      throw std::invalid_argument("min must not be greater than max");
    }
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
  }

  // Generate a random integer using normal distribution (mean and deviation)
  int getNormalInt(const int mean, const int dev) {
    std::normal_distribution<double> dist(mean, dev);
    const double result = dist(rng);

    // Ensure result is within valid integer range
    if (result < std::numeric_limits<int>::min())
      return std::numeric_limits<int>::min();
    if (result > std::numeric_limits<int>::max())
      return std::numeric_limits<int>::max();

    return static_cast<int>(std::round(result));
  }

  // Generate a random double between min and max (inclusive)
  double getRandomDouble(const double min, const double max) {
    if (min > max) {
      throw std::invalid_argument("min must not be greater than max");
    }
    std::uniform_real_distribution<double> dist(min, max);
    return dist(rng);
  }

  // Generate a random integer between min and max (inclusive)
  unsigned long long getRandomUll(const unsigned long long min,
                                  const unsigned long long max) {
    if (min > max) {
      throw std::invalid_argument("min must not be greater than max");
    }
    std::uniform_int_distribution<unsigned long long> dist(min, max);
    return dist(rng);
  }

  // Generate a random integer using binomial distribution
  int getBinomial(const int n, const double p) {
    if (n < 0 || p < 0.0 || p > 1.0) {
      throw std::invalid_argument("Invalid arguments for binomial distribution");
    }
    std::binomial_distribution<int> dist(n, p);
    return dist(rng);
  }

private:
  std::mt19937 rng; // Mersenne Twister 19937 generator
};

inline thread_local RandomGenerator
    rng; // global declaration for multithreading

#endif // RANDOMGENERATOR_H_INCLUDED
