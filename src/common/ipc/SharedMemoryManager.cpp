#include "common/ipc/SharedMemoryManager.h"

#include "common/output/Logger.h"
#include <cstring>
#include <stdexcept>

/**
 *  Get the shared memory instance.
 *
 * @return The shared memory instance.
 */
SharedMemoryManager &SharedMemoryManager::shared() {
  static SharedMemoryManager instance;
  return instance;
}

/**
 * Destructor for the shared memory manager.
 */
SharedMemoryManager::~SharedMemoryManager() {
  if (data_ != nullptr && data_ != (SharedState *)-1) {
    int result = shmdt(data_);
    if (result == -1) {
      std::string errorMessage = "Failed to detach from shared memory: " +
                                 std::string(std::strerror(errno));
      perror(errorMessage.c_str());
    }
    data_ = nullptr;
  }
}

/**
 * Initialize the shared memory manager.
 *
 * @param count The number of candidates.
 * @throw std::runtime_error If the shared memory cannot be initialized.
 */
void SharedMemoryManager::initialize(int count) {
  Logger::info("SharedMemoryManager::initialize(" + std::to_string(count) +
               ")");
  key_t key = getKey();
  Logger::info("Initializing shared memory for key: " + std::to_string(key));

  int tempId = shmget(key, 1, 0600);
  if (tempId != -1) {
    Logger::warn("Shared memory already exists. Cleaning up...");
    if (shmctl(tempId, IPC_RMID, NULL) == -1) {
      throw std::runtime_error(
          "Failed to clean up already existing shared memory");
    }
  }

  size_t size = getSize(count);
  Logger::info("Creating new shared memory with size: " + std::to_string(size) +
               " bytes");
  shared().shmId_ = shmget(key, size, IPC_CREAT | IPC_EXCL | 0600);
  if (shared().shmId_ == -1) {
    throw std::runtime_error("Failed to create new shared memory");
  }

  shared().data_ = (SharedState *)shmat(shared().shmId_, nullptr, 0);
  if (shared().data_ == (SharedState *)-1) {
    throw std::runtime_error("Failed to attach to shared memory");
  }

  memset(shared().data_, 0, size);
}

/**
 * Attach to the shared memory.
 *
 * @throw std::runtime_error If the shared memory cannot be attached.
 */
void SharedMemoryManager::attach() {
  key_t key = getKey();
  Logger::info("Attaching to shared memory with key: " + std::to_string(key));

  Logger::info("Getting shared memory for key: " + std::to_string(key));
  int shmId = shmget(key, 0, 0600);
  if (shmId == -1) {
    throw std::runtime_error("Failed to get shared memory for key: " +
                             std::to_string(key));
  }

  shared().data_ = (SharedState *)shmat(shmId, NULL, 0);
  if (shared().data_ == (SharedState *)-1) {
    throw std::runtime_error("Failed to attach to shared memory");
  }
}

/**
 * Detach from the shared memory.
 *
 * @throw std::runtime_error If the shared memory cannot be detached.
 */
void SharedMemoryManager::detach() {
  Logger::info("SharedMemoryManager::detach()");
  Logger::info("Detaching from shared memory");

  if (shared().data_ != (SharedState *)-1) {
    if (shmdt(shared().data_) == -1) {
      throw std::runtime_error("Failed to detach from shared memory");
    }
    shared().data_ = nullptr;
  }
}

/**
 * Destroy the shared memory.
 *
 * @throw std::runtime_error If the shared memory cannot be destroyed.
 */
void SharedMemoryManager::destroy() {
  Logger::info("SharedMemoryManager::destroy()");
  Logger::info("Destroying shared memory with id: " +
               std::to_string(shared().shmId_));

  if (shared().shmId_ != -1) {
    if (shmctl(shared().shmId_, IPC_RMID, NULL) == -1) {
      throw std::runtime_error("Failed to destroy shared memory");
    }
    shared().shmId_ = -1;
  }
}

/**
 * Get the data from the shared memory.
 *
 * @return The data from the shared memory.
 */
SharedState *SharedMemoryManager::data() { return shared().data_; }

/**
 * Get the key for the shared memory.
 *
 * @return The key for the shared memory.
 */
key_t SharedMemoryManager::getKey() {
  key_t key = ftok("/tmp", 155);
  if (key == -1) {
    throw std::runtime_error("Failed to get key for shared memory");
  }
  return key;
}

/**
 * Get the size of the shared memory.
 *
 * @param count The number of candidates.
 * @return The size of the shared memory.
 */
size_t SharedMemoryManager::getSize(int count) {
  return sizeof(SharedState) + sizeof(CandidateInfo) * count;
}
