#pragma once

#include <unordered_map>
#include <unistd.h>

enum ProcessType {
  MAIN = 0,
  CANDIDATE = 1,
  EXAMINER = 2,
};

class Config {
public:
  static Config &shared();
  void initializeFromArgs(int candidateCount, int startTime);
  void registerProcess(pid_t pid, ProcessType type);

  // Configuration based on args
  int candidateCount;
  int startTime;
  // Randomized configuration
  double candidateRatio;
  double examFailureRate;
  double reattemptRate;
  // Simulation state
  std::unordered_map<pid_t, ProcessType> processTypes = {};

private:
  double randomDouble(double min, double max);
};
