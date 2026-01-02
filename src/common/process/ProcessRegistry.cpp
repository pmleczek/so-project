#include "common/process/ProcessRegistry.h"

#include "common/ipc/SharedMemoryManager.h"
#include "common/output/Logger.h"
#include <signal.h>

void ProcessRegistry::registerCommission(pid_t pid, char commission) {
  if (commission == 'A') {
    SharedMemoryManager::data()->commissionAPID = pid;
  } else {
    SharedMemoryManager::data()->commissionBID = pid;
  }
}

void ProcessRegistry::unregister(pid_t pid) {
  if (pid == SharedMemoryManager::data()->commissionAPID) {
    SharedMemoryManager::data()->commissionAPID = -1;
  } else if (pid == SharedMemoryManager::data()->commissionBID) {
    SharedMemoryManager::data()->commissionBID = -1;
  } else {
    for (int i = 0; i < SharedMemoryManager::data()->candidateCount; i++) {
      if (SharedMemoryManager::data()->candidates[i].pid == pid) {
        SharedMemoryManager::data()->candidates[i].status = Terminated;
        return;
      }
    }

    Logger::error("Candidate process with PID:" + std::to_string(pid) +
                  " not found");
  }
}

void ProcessRegistry::propagateSignal(int signal) {
  if (SharedMemoryManager::data()->commissionAPID != -1) {
    kill(SharedMemoryManager::data()->commissionAPID, signal);
  }

  if (SharedMemoryManager::data()->commissionBID != -1) {
    kill(SharedMemoryManager::data()->commissionBID, signal);
  }

  for (int i = 0; i < SharedMemoryManager::data()->candidateCount; i++) {
    if (SharedMemoryManager::data()->candidates[i].status != Terminated) {
      kill(SharedMemoryManager::data()->candidates[i].pid, signal);
    }
  }
}
