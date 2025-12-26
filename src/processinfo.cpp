#include "processinfo.h"
#include "logger.h"

std::unordered_map<pid_t, ProcessType> ProcessInfo::processTypes;

void ProcessInfo::registerProcess(pid_t pid, ProcessType type) {
  Logger::shared().log("Registering process with PID: " + std::to_string(pid) +
                       " as type: " + getProcessTypeName(type));
  processTypes[pid] = type;
}

ProcessType ProcessInfo::getProcessType(pid_t pid) { return processTypes[pid]; }

std::string ProcessInfo::getProcessTypeName(ProcessType type) {
  return processTypeNames[type];
}

bool ProcessInfo::isMainProcessRegistered() {
  return processTypes.find(ProcessType::MAIN) != processTypes.end();
}

bool ProcessInfo::isCandidateProcess(pid_t pid) {
  return getProcessType(pid) == ProcessType::CANDIDATE;
}

bool ProcessInfo::isExaminerProcess(pid_t pid) {
  return getProcessType(pid) == ProcessType::EXAMINER;
}