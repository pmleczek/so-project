#pragma once

#include <string>

class Logger {
public:
  static Logger &shared();
  void log(const std::string &message);
  static void setupLogFile();

private:
  Logger();
  ~Logger();

  std::string getDateTime();
  std::string getLogMessage(const std::string &message);
  std::string getProcessIdentifier();

  int fileHandle_;
};
