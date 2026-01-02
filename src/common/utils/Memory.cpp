#include "common/utils/Memory.h"

#include "common/ipc/SharedMemoryManager.h"
#include "common/output/Logger.h"
#include <cstring>
#include <string>

void Memory::resetSeat(char commission, size_t seat) {
  if (seat >= 3) {
    Logger::warn("Tried to reset invalid seat with index: " +
                 std::to_string(seat));
    return;
  }

  if (commission == 'A') {
    SharedMemoryManager::data()->commissionA.seats[seat].pid = -1;
    SharedMemoryManager::data()->commissionA.seats[seat].questionsCount = 0;
    SharedMemoryManager::data()->commissionA.seats[seat].answered = false;
  } else if (commission == 'B') {
    SharedMemoryManager::data()->commissionB.seats[seat].pid = -1;
    SharedMemoryManager::data()->commissionB.seats[seat].questionsCount = 0;
    SharedMemoryManager::data()->commissionB.seats[seat].answered = false;
  }
}

void Memory::initializeMutex() {
  pthread_mutexattr_t attr;
  int result = pthread_mutexattr_init(&attr);
  if (result != 0) {
    std::string errorMessage =
        std::string(
            "Failed to initialize mutex attributes for shared memory: ") +
        std::strerror(result);
    throw std::runtime_error(errorMessage);
  }

  result = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  if (result != 0) {
    std::string errorMessage =
        std::string("Failed to set process shared attribute for mutex ") +
        "attributes for shared memory: " + std::strerror(result);

    throw std::runtime_error(errorMessage);
  }

  result = pthread_mutex_init(&SharedMemoryManager::data()->seatsMutex, &attr);
  if (result != 0) {
    std::string errorMessage =
        std::string("Failed to initialize mutex for shared memory: ") +
        std::strerror(result);
    pthread_mutexattr_destroy(&attr);
    throw std::runtime_error(errorMessage);
  }

  result = pthread_mutexattr_destroy(&attr);
  if (result != 0) {
    std::string errorMessage =
        std::string("Failed to destroy mutex attributes for shared memory: ") +
        std::strerror(result);
    Logger::warn(errorMessage);
  }
}
