#pragma once

#include "common/ipc/SharedState.h"
#include <sys/ipc.h>
#include <sys/shm.h>

class SharedMemoryManager {
public:
  SharedMemoryManager() = default;
  ~SharedMemoryManager();

  static SharedMemoryManager &shared();
  static void initialize(int count);
  static void destroy();
  static void attach();
  static void detach();
  static SharedState* data();

private:
  static key_t getKey();
  static size_t getSize(int count);

  int shmId_ = -1;
  SharedState* data_ = nullptr;
};
