#include "common/ipc/SemaphoreManager.h"
#include <cerrno>
#include <cstring>
#include <fcntl.h>

sem_t *SemaphoreManager::create(const std::string &name, int initialValue) {
  sem_t *sem = sem_open(name.c_str(), O_CREAT | O_EXCL, 0666, initialValue);
  if (sem == SEM_FAILED) {
    if (errno == EEXIST) {
      sem_unlink(name.c_str());
      sem = sem_open(name.c_str(), O_CREAT | O_EXCL, 0666, initialValue);
    }
    if (sem == SEM_FAILED) {
      throw std::runtime_error("Failed to create semaphore " + name + ": " +
                               strerror(errno));
    }
  }
  return sem;
}

sem_t *SemaphoreManager::open(const std::string &name) {
  sem_t *sem = sem_open(name.c_str(), 0);
  if (sem == SEM_FAILED) {
    throw std::runtime_error("Failed to open semaphore " + name + ": " +
                             strerror(errno));
  }
  return sem;
}

void SemaphoreManager::close(sem_t *sem) {
  if (sem != nullptr && sem != SEM_FAILED) {
    sem_close(sem);
  }
}

void SemaphoreManager::unlink(const std::string &name) {
  sem_unlink(name.c_str());
}

void SemaphoreManager::wait(sem_t *sem) {
  if (sem_wait(sem) == -1) {
    throw std::runtime_error("sem_wait failed: " +
                             std::string(strerror(errno)));
  }
}

void SemaphoreManager::post(sem_t *sem) {
  if (sem_post(sem) == -1) {
    throw std::runtime_error("sem_post failed: " +
                             std::string(strerror(errno)));
  }
}
