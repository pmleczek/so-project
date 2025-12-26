#pragma once

#include <string>

class Logger {
public:
  void log(const std::string &message);
  static Logger &shared();

private:
  std::string getDateTime();
};
