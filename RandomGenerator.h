#ifndef RANDOMGENERATOR_H_INCLUDED
#define RANDOMGENERATOR_H_INCLUDED

#include <iostream>
#include <random>   // For mt19937 and uniform distributions
#include <chrono>   // For seeding with system clock

class RandomGenerator {
public:
    RandomGenerator() {
        auto seed = std::chrono::system_clock::now().time_since_epoch().count();
        rng.seed(static_cast<unsigned long>(seed));
    }

    // Generate a random integer between min and max (inclusive)
    int getRandomInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(rng);
    }

    // Generate a random double between min and max (inclusive)
    double getRandomDouble(double min, double max) {
        std::uniform_real_distribution<double> dist(min, max);
        return dist(rng);
    }

private:
    std::mt19937 rng; // Mersenne Twister 19937 generator
};


#endif // RANDOMGENERATOR_H_INCLUDED
