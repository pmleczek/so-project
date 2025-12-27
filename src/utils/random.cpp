#include "utils/random.h"
#include <random>
#include <iostream>

double Random::randomDouble(double min, double max) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dis(min, max);
  return dis(gen);
}

int Random::randomInt(int min, int max) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(min, max);
  return dis(gen);
}

std::unordered_set<int> Random::randomValues(int count, int min, int max,
                                             std::unordered_set<int> exclude) {
  std::cout << "Generating " << count << " random values between " << min
            << " and " << max << " excluding " << exclude.size() << " values"
            << std::endl;
  std::unordered_set<int> values;
  while (values.size() < count) {
    int value = randomInt(min, max);
    if (exclude.count(value) == 0 && values.count(value) == 0) {
      values.insert(value);
    }
  }
  return values;
}
