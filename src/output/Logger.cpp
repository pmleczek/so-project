#include "output/Logger.h"

#include "utils/constants.h"
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <sys/file.h>
#include <unistd.h>

Logger &Logger::shared() {
  static Logger instance;
  return instance;
}

Logger::Logger() {
  std::string outputDirectory = Constants::outputDirectory();
  if (!std::filesystem::exists(outputDirectory)) {
    std::filesystem::create_directory(outputDirectory);
  }

  fileHandle_ = open(Constants::logFilePath().c_str(),
                     O_CREAT | O_WRONLY | O_APPEND, 0644);
  if (fileHandle_ == -1) {
    std::cerr << "Failed to open log file: " << Constants::logFilePath()
              << std::endl;
    // TODO: Add proper error handling
  }
}

Logger::~Logger() {
  if (fileHandle_ != -1) {
    close(fileHandle_);
    fileHandle_ = -1;
  }
}

void Logger::log(const std::string &message) {
  if (fileHandle_ == -1) {
    std::cerr << "Log file: " << Constants::logFilePath() << " is not open"
              << std::endl;
    // TODO: Add proper error handling
    return;
  }

  if (flock(fileHandle_, LOCK_EX) == -1) {
    std::cerr << "Failed to lock log file: " << Constants::logFilePath()
              << std::endl;
    // TODO: Add proper error handling
    return;
  }

  std::string logMessage = getLogMessage(message);
  ssize_t bytesWritten = write(fileHandle_, logMessage.c_str(), logMessage.length());
  if (bytesWritten == -1) {
    std::cerr << "Failed to write to log file: " << Constants::logFilePath()
              << std::endl;
    // TODO: Add proper error handling
    return;
  }

  fsync(fileHandle_);
  flock(fileHandle_, LOCK_UN);
}

void Logger::setupLogFile() {
  if (std::filesystem::exists(Constants::logFilePath())) {
    std::filesystem::remove(Constants::logFilePath());
  }
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

std::string Logger::getProcessIdentifier() {
  return "[" + std::to_string(getpid()) + "]";
}

std::string Logger::getLogMessage(const std::string &message) {
  return getDateTime() + " " + getProcessIdentifier() + " " + message + "\n";
}
