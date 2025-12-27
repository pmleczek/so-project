#pragma once

#include "SemaphoreOperations.h"
#include <sys/ipc.h>
#include <sys/sem.h>

class SemaphoreManager {
public:
  static SemaphoreManager &shared();
  bool initialize(int candidateCount);
  bool attach();
  void wait(int semop);
  void signal(int semop);
  void destroy();

private:
  SemaphoreManager() = default;
  ~SemaphoreManager();

  key_t getSemKey();
  int getNumSemaphores(int candidateCount);

  int semId_ = -1;
  int numSemaphores_ = 0;
  static const int PROJECT_IDENTIFIER = 156;
};
