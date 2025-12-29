#pragma once

#include "common/ipc/SemaphoreManager.h"
#include "common/ipc/SharedState.h"

struct ThreadData {
  int memberId;
  char commissionId;
  bool *running;
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
  CandidateInfo &findCandidate(int seat);
  static void *threadFunction(void *arg);

  int memberCount_;
  char commissionType_;
  std::atomic<bool> running = true;
  sem_t *semaphore;
  pthread_t threadIds[3];
  ThreadData threadData[3];
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  std::atomic<int> candidatesProcessed = 0;
};
