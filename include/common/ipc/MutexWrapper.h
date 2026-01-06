#pragma once

#include <pthread.h>

class MutexWrapper {
public:
  static void lock(pthread_mutex_t *mutex);
  static void unlock(pthread_mutex_t *mutex);
};
