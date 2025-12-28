#include "common/ipc/SharedMemoryManager.h"
#include "common/output/Logger.h"
#include <cstring>

SharedMemoryManager &SharedMemoryManager::shared() {
  static SharedMemoryManager instance;
  return instance;
}

SharedMemoryManager::~SharedMemoryManager() { shared().detach(); }

void SharedMemoryManager::initialize(int count) {
  Logger::info("SharedMemoryManager::initialize(" + std::to_string(count) +
               ")");
  key_t key = getKey();
  Logger::info("Initializing shared memory for key: " + std::to_string(key));

  int tempId = shmget(key, 1, 0600);
  if (tempId != -1) {
    Logger::warn("Shared memory already exists. Cleaning up...");
    if (shmctl(tempId, IPC_RMID, NULL) == -1) {
      Logger::error("Failed to clean up shared memory");
      // TODO: Handle error
      exit(1);
    }
  }

  size_t size = getSize(count);
  Logger::info("Creating new shared memory with size: " + std::to_string(size) +
               " bytes");
  shared().shmId_ = shmget(key, size, IPC_CREAT | IPC_EXCL | 0600);
  if (shared().shmId_ == -1) {
    Logger::error("Failed to create shared memory");
    Logger::error("Error: " + std::string(strerror(errno)));
    // TODO: Handle error
    exit(1);
  }

  shared().data_ = (SharedState *)shmat(shared().shmId_, nullptr, 0);
  if (shared().data_ == (SharedState *)-1) {
    // TODO: Handle error
    Logger::error("Failed to attach to shared memory");
    exit(1);
  }

  memset(shared().data_, 0, size);
}

void SharedMemoryManager::attach() {
  key_t key = getKey();
  Logger::info("Attaching to shared memory with key: " + std::to_string(key));

  Logger::info("Getting shared memory for key: " + std::to_string(key));
  int shmId = shmget(key, 0, 0600);
  if (shmId == -1) {
    Logger::error("Failed to get shared memory for key: " +
                  std::to_string(key));
    Logger::error("Error: " + std::string(strerror(errno)));
    // TODO: Handle error
    exit(1);
  }

  shared().data_ = (SharedState *)shmat(shmId, NULL, 0);
  if (shared().data_ == (SharedState *)-1) {
    Logger::error("Failed to attach to shared memory");
    // TODO: Handle error
    exit(1);
  }
}

void SharedMemoryManager::detach() {
  Logger::info("SharedMemoryManager::detach()");
  Logger::info("Detaching from shared memory");

  if (shared().data_ != (SharedState *)-1) {
    shmdt(shared().data_);
    shared().data_ = nullptr;
  }
}

void SharedMemoryManager::destroy() {
  Logger::info("SharedMemoryManager::destroy()");
  Logger::info("Destroying shared memory with id: " +
               std::to_string(shared().shmId_));

  if (shared().shmId_ != -1) {
    if (shmctl(shared().shmId_, IPC_RMID, NULL) == -1) {
      Logger::error("Failed to destroy shared memory");
      Logger::error("Error: " + std::string(strerror(errno)));
      // TODO: Handle error
      exit(1);
    }
    shared().shmId_ = -1;
  }
}

SharedState *SharedMemoryManager::data() { return shared().data_; }

key_t SharedMemoryManager::getKey() { return ftok("/tmp", 155); }

size_t SharedMemoryManager::getSize(int count) {
  return sizeof(SharedState) + sizeof(CandidateInfo) * count + sizeof(ComissionQueue) * 2;
}