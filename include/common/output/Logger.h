#pragma once

#include <string>

class Logger {
public:
  static Logger &shared();
  static void setupLogFile();
  static void setProcessPrefix(const std::string &prefix);

  static void info(const std::string &message);
  static void error(const std::string &message);
  static void warn(const std::string &message);

private:
  Logger();
  ~Logger();

  std::string getPrefix(const std::string &logLevel);
  void log(const std::string &message, const std::string &logLevel);

  int fileHandle_;
  std::string processPrefix_;
};
