#pragma once

#include <unordered_set>

class Random {
public:
  static double randomDouble(double min, double max);
  static int randomInt(int min, int max);
  static std::unordered_set<int> randomInts(int count, int min, int max,
                                            std::unordered_set<int> exclude = {});
};
