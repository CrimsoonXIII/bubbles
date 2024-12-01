#pragma once
#include <random>
class Random {
private:
    std::mt19937 generator;
    std::uniform_real_distribution<float> distribution;

public:
    Random(unsigned int seed)
        : generator(seed), distribution(0.0f, 1.0f) {}

    float random() {
        return distribution(generator);
    }
};