#pragma once

#include <unordered_map>
#include <unistd.h>

class Config {
public:
  static Config &shared();
  void initializeFromArgs(int candidateCount, int startTime);

  // Configuration based on args
  int candidateCount;
  int startTime;
  // Randomized configuration
  double candidateRatio;
  double examFailureRate;
  double reattemptRate;
};
