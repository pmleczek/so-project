#pragma once

#include "common/ipc/SharedMemoryManager.h"
#include <cstddef>

class Memory {
public:
  static void resetSeat(char commission, size_t seat);
  static void initializeMutex();
  static CandidateInfo *findCandidate(char commissionType, int seat);
};
