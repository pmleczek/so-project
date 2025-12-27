#include "utils/random.h"
#include <random>

double Random::randomDouble(double min, double max) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dis(min, max);
  return dis(gen);
}
