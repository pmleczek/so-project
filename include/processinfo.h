#pragma once

#include <string>
#include <unistd.h>
#include <unordered_map>

enum ProcessType {
  MAIN = 0,
  CANDIDATE = 1,
  EXAMINER = 2,
};

class ProcessInfo {
public:
  static void registerProcess(pid_t pid, ProcessType type);
  static ProcessType getProcessType(pid_t pid);
  static std::string getProcessTypeName(ProcessType type);
  static bool isMainProcessRegistered();
  static bool isCandidateProcess(pid_t pid);
  static bool isExaminerProcess(pid_t pid);

private:
  static std::unordered_map<pid_t, ProcessType> processTypes;
  static constexpr const char *processTypeNames[] = {
      "main",
      "candidate",
      "examiner",
  };
};
