#include "common/utils/Misc.h"

#include <cstring>
#include <errno.h>
#include <stdexcept>
#include <string>
#include <unistd.h>

void Misc::safeSleep(int seconds) {
  int result = sleep(seconds);
  if (result < 0) {
    throw std::runtime_error("sleep() failed: " +
                             std::string(std::strerror(errno)));
  }
}

void Misc::safeUSleep(int microseconds) {
  int result = usleep(microseconds);
  if (result < 0) {
    throw std::runtime_error("usleep() failed: " +
                             std::string(std::strerror(errno)));
  }
}