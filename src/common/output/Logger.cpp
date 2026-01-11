#include "common/output/Logger.h"

#include "common/ipc/SemaphoreManager.h"
#include <cerrno>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

Logger &Logger::shared() {
  static Logger instance;
  return instance;
}

Logger::Logger() {
  const char *outputDirectory = "../output";
  struct stat st;
  if (stat(outputDirectory, &st) == -1) {
    if (errno == ENOENT) {
      if (mkdir(outputDirectory, 0755) == -1) {
        throw std::runtime_error("Failed to create output directory: " +
                                 std::string(strerror(errno)));
      }
    } else {
      throw std::runtime_error("Failed to stat output directory: " +
                               std::string(strerror(errno)));
    }
  }

  fileHandle_ =
      open("../output/simulation.log", O_CREAT | O_WRONLY | O_APPEND, 0644);
  if (fileHandle_ == -1) {
    throw std::runtime_error("Failed to open log file: " +
                             std::string(strerror(errno)));
  }
}

Logger::~Logger() {
  if (logSemaphore_ != nullptr) {
    try {
      SemaphoreManager::close(logSemaphore_);
    } catch (const std::exception &e) {
      throw std::runtime_error("Failed to close logger semaphore: " +
                               std::string(e.what()));
    }
  }

  if (fileHandle_ != -1) {
    if (close(fileHandle_) == -1) {
      throw std::runtime_error("Failed to close log file: " +
                               std::string(strerror(errno)));
    }
    fileHandle_ = -1;
  }
}

void Logger::setProcessPrefix(const std::string &prefix) {
  shared().processPrefix_ = prefix;
}

void Logger::info(const std::string &message) { shared().log(message, "INFO"); }

void Logger::error(const std::string &message) {
  shared().log(message, "ERROR");
}

void Logger::warn(const std::string &message) { shared().log(message, "WARN"); }

void Logger::log(const std::string &message, const std::string &logLevel) {
  if (fileHandle_ == -1) {
    throw std::runtime_error("Log file is not open");
    return;
  }

  if (logSemaphore_ == nullptr) {
    try {
      logSemaphore_ = SemaphoreManager::open("logger");
    } catch (const std::exception &e) {
      return;
    }
  }

  try {
    SemaphoreManager::wait(logSemaphore_);
  } catch (const std::exception &e) {
    throw std::runtime_error("Failed to acquire logger semaphore: " +
                             std::string(e.what()));
    return;
  }

  std::string logMessage = getPrefix(logLevel) + " " + message + "\n";

  ssize_t bytesWritten =
      write(fileHandle_, logMessage.c_str(), logMessage.length());
  if (bytesWritten == -1) {
    throw std::runtime_error("Failed to write to log file: " +
                             std::string(strerror(errno)));
    try {
      SemaphoreManager::post(logSemaphore_);
    } catch (const std::exception &e) {
      throw std::runtime_error("Failed to release logger semaphore: " +
                               std::string(e.what()));
    }
    return;
  }

  if (fsync(fileHandle_) == -1) {
    throw std::runtime_error("Failed to fsync log file: " +
                             std::string(strerror(errno)));
  }

  try {
    SemaphoreManager::post(logSemaphore_);
  } catch (const std::exception &e) {
    throw std::runtime_error("Failed to release logger semaphore: " +
                             std::string(e.what()));
  }
}

void Logger::setupLogFile() {
  if (unlink("../output/simulation.log") == -1 && errno != ENOENT) {
    throw std::runtime_error("Failed to remove existing log file: " +
                             std::string(strerror(errno)));
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

  if (!processPrefix_.empty()) {
    ss << " [" << processPrefix_ << "]";
  }

  return ss.str();
}
