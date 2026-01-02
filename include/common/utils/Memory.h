#pragma once

#include <cstddef>

class Memory {
public:
  static void resetSeat(char commission, size_t seat);
  static void initializeMutex();
};
