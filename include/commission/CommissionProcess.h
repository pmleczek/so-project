#pragma once

#include "common/ipc/SemaphoreManager.h"
#include "common/ipc/SharedState.h"
#include <atomic>

struct ThreadData {
  int memberId;
  char commissionId;
  std::atomic<bool> *running;
  pthread_mutex_t *mutex;
};

class CommissionProcess {
public:
  CommissionProcess(int argc, char *argv[]);
  void waitForExamStart();
  void start();
  void cleanup();

private:
  void initialize(int argc, char *argv[]);
  void mainLoop();
  void spawnThreads();
  void waitThreads();
  CandidateInfo *findCandidate(int seat);
  static void *threadFunction(void *arg);

  int memberCount_;
  char commissionType_;
  std::atomic<bool> running = true;
  sem_t *semaphore;
  pthread_t threadIds[5];
  ThreadData threadData[5];
  std::atomic<int> candidatesProcessed = 0;
};
