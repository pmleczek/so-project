#include "logger.h"

#include <iostream>

Logger &Logger::shared() {
  static Logger instance;
  return instance;
}

void Logger::log(const std::string &message) {
  // TODO: Implement logging to file
  std::cout << getDateTime() << " " << message << std::endl;
}

std::string Logger::getDateTime() {
  auto now = std::chrono::system_clock::now();
  std::time_t time = std::chrono::system_clock::to_time_t(now);
  std::tm *timeinfo = std::localtime(&time);

  char buffer[80];
  std::strftime(buffer, sizeof(buffer), "[%Y-%m-%d %H:%M:%S", timeinfo);

  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;

  std::string result(buffer);
  result += "." + std::to_string(ms.count()) + "]";

  return result;
}