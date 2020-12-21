#include <cmath>
#include <iostream>
#include <random>

#include <unity.h>

#include "rng.h"

template <typename TDistribution>
double sample_mean(TDistribution dist, int sample_size = 10000)
{
    pcg mt; // Mersenne Twister generator
    double sum = 0;
    for (int i = 0; i < sample_size; ++i)
        sum += dist(mt);
    return sum / sample_size;
}

template <typename TDistribution>
void test_mean(TDistribution dist, std::string name, double true_mean, double true_variance)
{
    double true_stdev = sqrt(true_variance);
    std::cout << "Testing " << name << " distribution\n";
    int sample_size = 1000000;
    double mean = sample_mean(dist, sample_size);
    std::cout << "Computed mean: " << mean << "\n";
    double lower = true_mean - true_stdev/sqrt((double)sample_size);
    double upper = true_mean + true_stdev/sqrt((double)sample_size);
    std::cout << "Expected a value between " << lower << " and " << upper << "\n\n";
}

void test_uniform(void) {
    std::uniform_int_distribution<int16_t> uniform(-4096, 4096);
    test_mean(uniform, "uniform", 0, 4096);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_uniform);
    UNITY_END();
}
