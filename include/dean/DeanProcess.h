#pragma once

#include "common/process/BaseProcess.h"
#include "dean/DeanConfig.h"
#include <unordered_set>

class DeanProcess : public BaseProcess {
public:
  DeanProcess(int argc, char *argv[]);

  void validateArguments(int argc, char *argv[]) override;
  void initialize() override;
  void cleanup() override;
  void setupSignalHandlers() override;
  void handleError(const char *message) override;

  void spawnComissions();
  void spawnCandidates();
  void verifyCandidates();
  void waitForExamStart();
  void start();

private:
  std::unordered_set<int> getFailedExamIndices();
  std::unordered_set<int>
  getRetakeExamIndices(std::unordered_set<int> excludedIndices);
  static void evacuationHandler(int signal);
  static void terminationHandler(int signal);
  static void *cleanupThreadFunction(void *arg);

  int candidateCount;
  int retaking = 0;
  DeanConfig config;

  std::vector<pid_t> childPids;
  pthread_mutex_t childPidsMutex;
  pthread_t cleanupThread;
};
