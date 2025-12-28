#include "common/output/Logger.h"

#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <sys/file.h>
#include <unistd.h>

Logger &Logger::shared() {
  static Logger instance;
  return instance;
}

Logger::Logger() {
  std::string outputDirectory = "../output";
  if (!std::filesystem::exists(outputDirectory)) {
    std::filesystem::create_directory(outputDirectory);
  }

  fileHandle_ =
      open("../output/simulation.log", O_CREAT | O_WRONLY | O_APPEND, 0644);
  if (fileHandle_ == -1) {
    std::cerr << "Failed to open log file: ../output/simulation.log"
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

void Logger::info(const std::string &message) {
  shared().log(message, "INFO");
}

void Logger::error(const std::string &message) {
  shared().log(message, "ERROR");
}

void Logger::warn(const std::string &message) {
  shared().log(message, "WARN");
}

void Logger::log(const std::string &message, const std::string &logLevel) {
  if (fileHandle_ == -1) {
    std::cerr << "Log file: ../output/simulation.log is not open" << std::endl;
    // TODO: Add proper error handling
    return;
  }

  if (flock(fileHandle_, LOCK_EX) == -1) {
    std::cerr << "Failed to lock log file: ../output/simulation.log"
              << std::endl;
    // TODO: Add proper error handling
    return;
  }

  std::string logMessage = getPrefix(logLevel) + " " + message;

  ssize_t bytesWritten =
      write(fileHandle_, logMessage.c_str(), logMessage.length());
  if (bytesWritten == -1) {
    std::cerr << "Failed to write to log file: ../output/simulation.log"
              << std::endl;
    // TODO: Add proper error handling
    return;
  }

  fsync(fileHandle_);
  flock(fileHandle_, LOCK_UN);
}

void Logger::setupLogFile() {
  if (std::filesystem::exists("../output/simulation.log")) {
    std::filesystem::remove("../output/simulation.log");
  }
}

std::string Logger::getPrefix(const std::string &logLevel) {
  std::stringstream ss;

  // Log level
  ss << "[" << logLevel << "] ";

  // Date and time
  auto now = std::chrono::system_clock::now();
  std::time_t time = std::chrono::system_clock::to_time_t(now);
  std::tm *timeinfo = std::localtime(&time);

  char buffer[80];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;

  ss << "[" << buffer << "." << ms.count() << "]";

  return ss.str();
}
