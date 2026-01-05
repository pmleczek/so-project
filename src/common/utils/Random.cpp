#include "common/utils/Random.h"

#include <random>

double Random::randomDouble(double min, double max) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dist(min, max);
  return dist(gen);
}

int Random::randomInt(int min, int max) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(min, max);
  return dist(gen);
}

double Random::sampleMean(int samples, double min, double max) {
  double sum = 0.0;
  for (int i = 0; i < samples; i++) {
    sum += randomDouble(min, max);
  }

  return sum / samples;
}

std::unordered_set<int> Random::randomInts(int count, int min, int max,
                                           std::unordered_set<int> exclude) {
  std::unordered_set<int> result;
  while (result.size() < count) {
    int num = randomInt(min, max);
    if (result.find(num) == result.end() &&
        exclude.find(num) == exclude.end()) {
      result.insert(num);
    }
  }
  return result;
}
