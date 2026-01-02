#pragma once

#include <string>
#include <unistd.h>
#include <vector>

class BaseProcess {
public:
  BaseProcess(int argc, char *argv[], bool setupLogger = false);
  virtual ~BaseProcess() = default;

  virtual std::vector<int> validateArguments(int argc, char *argv[]) = 0;
  virtual void initialize(int argc, char *argv[]) = 0;
  virtual void cleanup() = 0;
  virtual void handleError(const char *message) = 0;
  virtual void setupSignalHandlers() = 0;
  virtual void handleTermination(int signal);

  static std::string getProcessName(int argc, char *argv[]);

  std::string processName_;

protected:
  static BaseProcess *instance_;
  pid_t pid_;

  static void terminationHandler(int signal);
};
