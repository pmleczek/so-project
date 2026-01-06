#include "common/ipc/MutexWrapper.h"

#include <cstring>
#include <stdexcept>
#include <string>

void MutexWrapper::lock(pthread_mutex_t *mutex) {
  int result = pthread_mutex_lock(mutex);
  if (result != 0) {
    throw std::runtime_error("pthread_mutex_lock failed: " +
                             std::string(std::strerror(result)));
  }
}

void MutexWrapper::unlock(pthread_mutex_t *mutex) {
  int result = pthread_mutex_unlock(mutex);
  if (result != 0) {
    throw std::runtime_error("pthread_mutex_unlock failed: " +
                             std::string(std::strerror(result)));
  }
}