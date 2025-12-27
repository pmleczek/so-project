#include "ipc/SharedMemoryManager.h"

#include "output/Logger.h"
#include <cerrno>
#include <cstring>

// TODO: Add logs
SharedMemoryManager &SharedMemoryManager::shared() {
  static SharedMemoryManager instance;
  return instance;
}

SharedMemoryManager::~SharedMemoryManager() { detach(); }

bool SharedMemoryManager::attach() {
  Logger::shared().log("SharedMemoryManager::attach()");
  shmId = shmget(SharedMemoryManager::PROJECT_IDENTIFIER, 0, 0600);
  if (shmId == -1) {
    // TODO: Handle error
    return false;
  }

  data_ = (SharedMemoryData *)shmat(shmId, nullptr, 0);
  if (data_ == (SharedMemoryData *)-1) {
    // TODO: Handle error
    return false;
  }

  return true;
}

void SharedMemoryManager::destroy() {
  Logger::shared().log("SharedMemoryManager::destroy()");
  detach();

  if (shmId != -1) {
    shmctl(shmId, IPC_RMID, nullptr);
    shmId = -1;
  }
}

void SharedMemoryManager::detach() {
  Logger::shared().log("SharedMemoryManager::detach()");
  if (data_ != nullptr) {
    shmdt(data_);
    data_ = nullptr;
  }
}

SharedMemoryData *SharedMemoryManager::data() { return data_; }

bool SharedMemoryManager::initialize(int candidateCount) {
  Logger::shared().log("SharedMemoryManager::initialize()");
  // TODO: Add logging
  size_t shmSize = getShmSize(candidateCount);

  shmId = shmget(SharedMemoryManager::PROJECT_IDENTIFIER, shmSize,
                 IPC_CREAT | IPC_EXCL | 0600);
  if (shmId == -1) {
    // TODO: Handle error
    return false;
  }

  data_ = (SharedMemoryData *)shmat(shmId, nullptr, 0);
  if (data_ == (SharedMemoryData *)-1) {
    // TODO: Handle error
    return false;
  }

  memset(data_, 0, shmSize);
  data_->candidateCount = candidateCount;

  return true;
}

size_t SharedMemoryManager::getShmSize(int candidateCount) {
  return sizeof(SharedMemoryData) +
         candidateCount * sizeof(SharedCandidateState);
}