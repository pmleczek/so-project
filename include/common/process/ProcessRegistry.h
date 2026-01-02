#pragma once

#include <unistd.h>

class ProcessRegistry {
public:
  static void registerCommission(pid_t pid, char commission);
  static void unregister(pid_t pid);
  static void propagateSignal(int signal);
};
