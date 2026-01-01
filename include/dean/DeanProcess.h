#pragma once

#include "dean/DeanConfig.h"
#include <unordered_set>

class DeanProcess {
public:
  DeanProcess(int argc, char *argv[]);
  void initialize(int argc, char *argv[]);
  void spawnComissions();
  void spawnCandidates();
  void verifyCandidates();
  void waitForExamStart();
  void start();
  void cleanup();

private:
  int assertPlaceCount(int argc, char *argv[]);
  int assertStartTime(int argc, char *argv[]);
  std::unordered_set<int> getFailedExamIndices();
  std::unordered_set<int>
  getRetakeExamIndices(std::unordered_set<int> excludedIndices);

  int candidateCount;
  int retaking = 0;
  DeanConfig config;
};