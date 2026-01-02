#pragma once

#include "common/process/BaseProcess.h"
#include "dean/DeanConfig.h"
#include <unordered_set>

class DeanProcess : public BaseProcess {
public:
  DeanProcess(int argc, char *argv[]);

  std::vector<int> validateArguments(int argc, char *argv[]) override;
  void initialize(int argc, char *argv[]) override;
  void cleanup() override;
  void setupSignalHandlers() override;
  void handleError(const char *message) override;

  void spawnComissions();
  void spawnCandidates();
  void verifyCandidates();
  void waitForExamStart();
  void start();

private:
  int assertPlaceCount(int argc, char *argv[]);
  int assertStartTime(int argc, char *argv[]);
  std::unordered_set<int> getFailedExamIndices();
  std::unordered_set<int>
  getRetakeExamIndices(std::unordered_set<int> excludedIndices);
  static void evacuationHandler(int signal);
  static void terminationHandler(int signal);

  int candidateCount;
  int retaking = 0;
  DeanConfig config;
};
