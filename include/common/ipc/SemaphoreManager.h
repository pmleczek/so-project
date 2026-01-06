#pragma once

#include <semaphore.h>
#include <stdexcept>
#include <string>

class SemaphoreManager {
public:
  static sem_t *create(const std::string &name, int initialValue);
  static sem_t *open(const std::string &name);
  static void close(sem_t *sem);
  static void unlink(const std::string &name);
  static void wait(sem_t *sem);
  static void post(sem_t *sem);
};