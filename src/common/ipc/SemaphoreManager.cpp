#include "common/ipc/SemaphoreManager.h"
#include <cerrno>
#include <cstring>
#include <fcntl.h>

/**
 * Create a semaphore.
 *
 * @param name The name of the semaphore.
 * @param initialValue The initial value of the semaphore.
 * @return The semaphore.
 * @throw std::runtime_error If the semaphore cannot be created.
 */
sem_t *SemaphoreManager::create(const std::string &name, int initialValue) {
  sem_t *sem = sem_open(name.c_str(), O_CREAT | O_EXCL, 0666, initialValue);

  if (sem == SEM_FAILED) {
    if (errno == EEXIST) {
      if (sem_unlink(name.c_str()) == -1 && errno != ENOENT) {
        throw std::runtime_error("Failed to unlink existing semaphore " + name +
                                 ": " + strerror(errno));
      }

      sem = sem_open(name.c_str(), O_CREAT | O_EXCL, 0666, initialValue);
    }

    if (sem == SEM_FAILED) {
      throw std::runtime_error("Failed to create semaphore " + name + ": " +
                               strerror(errno));
    }
  }

  return sem;
}

/**
 * Open a semaphore.
 *
 * @param name The name of the semaphore.
 * @return The semaphore.
 * @throw std::runtime_error If the semaphore cannot be opened.
 */
sem_t *SemaphoreManager::open(const std::string &name) {
  sem_t *sem = sem_open(name.c_str(), 0);

  if (sem == SEM_FAILED) {
    throw std::runtime_error("Failed to open semaphore " + name + ": " +
                             strerror(errno));
  }

  return sem;
}

/**
 * Close a semaphore.
 *
 * @param sem The semaphore to close.
 * @throw std::runtime_error If the semaphore cannot be closed.
 */
void SemaphoreManager::close(sem_t *sem) {
  if (sem != nullptr && sem != SEM_FAILED) {
    if (sem_close(sem) == -1) {
      throw std::runtime_error("Failed to close semaphore: " +
                               std::string(strerror(errno)));
    }
  }
}

/**
 * Unlink a semaphore.
 *
 * @param name The name of the semaphore.
 * @throw std::runtime_error If the semaphore cannot be unlinked.
 */
void SemaphoreManager::unlink(const std::string &name) {
  if (sem_unlink(name.c_str()) == -1 && errno != ENOENT) {
    throw std::runtime_error("Failed to unlink semaphore " + name + ": " +
                             std::string(strerror(errno)));
  }
}

/**
 * Wait on a semaphore.
 *
 * @param sem The semaphore to wait on.
 * @throw std::runtime_error If the semaphore cannot be waited on.
 */
void SemaphoreManager::wait(sem_t *sem) {
  if (sem_wait(sem) == -1) {
    throw std::runtime_error("Failed to wait on semaphore: " +
                             std::string(strerror(errno)));
  }
}

/**
 * Post to a semaphore.
 *
 * @param sem The semaphore to post to.
 * @throw std::runtime_error If the semaphore cannot be posted to.
 */
void SemaphoreManager::post(sem_t *sem) {
  if (sem_post(sem) == -1) {
    throw std::runtime_error("Failed to post to semaphore: " +
                             std::string(strerror(errno)));
  }
}
