#ifndef RANDOMGENERATOR_H_INCLUDED
#define RANDOMGENERATOR_H_INCLUDED

#include <iostream>
#include <random>   // For mt19937 and uniform distributions
#include <chrono>   // For seeding with system clock
#include <limits>

class RandomGenerator {
public:
    RandomGenerator() {
        //const auto seed = std::chrono::system_clock::now().time_since_epoch().count();
        //rng.seed(static_cast<unsigned long>(seed));
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
        if (result < std::numeric_limits<int>::min()) return std::numeric_limits<int>::min();
        if (result > std::numeric_limits<int>::max()) return std::numeric_limits<int>::max();

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
    unsigned long long getRandomUll(const int min, const int max) {
        if (min > max) {
            throw std::invalid_argument("min must not be greater than max");
        }
        std::uniform_int_distribution<unsigned long long> dist(min, max);
        return dist(rng);
    }

private:
    std::mt19937 rng; // Mersenne Twister 19937 generator
};

thread_local RandomGenerator rng; // global declaration for multithreading

#endif // RANDOMGENERATOR_H_INCLUDED
