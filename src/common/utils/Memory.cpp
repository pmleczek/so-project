#include "common/utils/Memory.h"

#include "common/ipc/SharedMemoryManager.h"
#include "common/output/Logger.h"
#include <cstring>
#include <string>
#include <stdexcept>

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
    throw std::runtime_error("Failed to initialize mutex attributes: " +
                             std::string(std::strerror(result)));
  }

  result = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  if (result != 0) {
    pthread_mutexattr_destroy(&attr);
    throw std::runtime_error("Failed to set process shared attribute: " +
                             std::string(std::strerror(result)));
  }

  SharedState *state = SharedMemoryManager::data();

  auto initMutex = [&](pthread_mutex_t *mutex, const char *name) {
    result = pthread_mutex_init(mutex, &attr);
    if (result != 0) {
      pthread_mutexattr_destroy(&attr);
      throw std::runtime_error(std::string("Failed to initialize ") + name +
                               " mutex: " + std::strerror(result));
    }
  };

  initMutex(&state->candidateMutex, "candidate");
  initMutex(&state->commissionAMutex, "commissionA");
  initMutex(&state->commissionBMutex, "commissionB");
  initMutex(&state->examStateMutex, "examState");

  result = pthread_mutexattr_destroy(&attr);
  if (result != 0) {
    Logger::warn("Failed to destroy mutex attributes: " +
                 std::string(std::strerror(result)));
  }
}

CandidateInfo *Memory::findCandidate(char commissionType, int seat) {
  SharedState *state = SharedMemoryManager::data();
  CommissionInfo *commission =
      commissionType == 'A' ? &state->commissionA : &state->commissionB;
  int pid = commission->seats[seat].pid;

  for (int i = 0; i < state->candidateCount; i++) {
    if (state->candidates[i].pid == pid) {
      return &state->candidates[i];
    }
  }

  Logger::warn("Candidate not found for seat " + std::to_string(seat) +
               " with pid " + std::to_string(pid) +
               " - candidate may have already exited");
  return nullptr;
}