#pragma once

#include "common/ipc/SemaphoreManager.h"
#include "common/ipc/SharedState.h"
#include "common/process/BaseProcess.h"
#include <atomic>

struct ThreadData {
  int memberId;
  char commissionId;
  std::atomic<bool> *running;
  pthread_mutex_t *mutex;
};

class CommissionProcess : public BaseProcess {
public:
  CommissionProcess(int argc, char *argv[]);

  void validateArguments(int argc, char *argv[]) override;
  void initialize() override;
  void cleanup() override;
  void setupSignalHandlers() override;
  void handleError(const char *message) override;

  void waitForExamStart();
  void start();
  static void terminationHandler(int signal);

private:
  void mainLoop();
  void spawnThreads();
  void waitThreads();
  static void *threadFunction(void *arg);

  int memberCount_;
  char commissionType_;
  std::atomic<bool> running = true;
  sem_t *semaphore;
  pthread_t threadIds[5];
  ThreadData threadData[5];
  std::atomic<int> candidatesProcessed = 0;
};
