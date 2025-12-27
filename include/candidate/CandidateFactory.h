#pragma once

#include "ipc/SharedMemoryManager.h"
#include <unistd.h>

class CandidateFactory {
public:
  static SharedCandidateState create(pid_t pid, bool passedExam);
};