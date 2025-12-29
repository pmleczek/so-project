#include "common/utils/Time.h"

#include <ctime>
#include <sstream>

int Time::seconds(const std::string &timeStr) {
  int h, m;
  char colon;
  std::istringstream iss(timeStr);
  iss >> h >> colon >> m;
  return h * 3600 + m * 60;
}

int Time::now() {
  std::time_t now = std::time(nullptr);
  std::tm *local = std::localtime(&now);
  return local->tm_hour * 3600 + local->tm_min * 60 + local->tm_sec;
}
