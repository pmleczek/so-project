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

  key_t shmKey = getShmKey();
  shmId = shmget(shmKey, 0, 0600);
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

  key_t shmKey = getShmKey();
  size_t shmSize = getShmSize(candidateCount);

  Logger::shared().log("Creating shared memory segment with key " +
                       std::to_string(shmKey) + " and size " +
                       std::to_string(shmSize));

  shmId = shmget(shmKey, shmSize, IPC_CREAT | IPC_EXCL | 0600);
  if (shmId == -1) {
    if (errno == EEXIST) {
      Logger::shared().log("Shared memory already exists");
      int oldShmId = shmget(shmKey, 0, 0600);
      if (oldShmId != -1) {
        if (shmctl(oldShmId, IPC_RMID, nullptr) == -1) {
          Logger::shared().log("WARNING: Failed to remove old shared memory: " +
                               std::string(strerror(errno)));
        } else {
          Logger::shared().log("Removed old shared memory segment");
        }
      }

      shmId = shmget(shmKey, shmSize, IPC_CREAT | IPC_EXCL | 0600);
    } else {
      // TODO: Handle error
      Logger::shared().log("WARNING: Failed to create shared memory: " +
                           std::string(strerror(errno)));
      return false;
    }
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

key_t SharedMemoryManager::getShmKey() {
  return ftok("/tmp", SharedMemoryManager::PROJECT_IDENTIFIER);
}

size_t SharedMemoryManager::getShmSize(int candidateCount) {
  return sizeof(SharedMemoryData) +
         candidateCount * sizeof(SharedCandidateState);
}