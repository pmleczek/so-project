#include "common/process/BaseProcess.h"

#include "common/output/Logger.h"

BaseProcess *BaseProcess::instance_ = nullptr;

BaseProcess::BaseProcess(int argc, char *argv[], bool setupLogger) {
  processName_ = getProcessName(argc, argv);
  pid_ = getpid();
  instance_ = this;

  if (setupLogger) {
    Logger::setupLogFile();
  }

  Logger::info("Process: " + processName_ +
               " started (pid=" + std::to_string(pid_) + ")");
}

void BaseProcess::handleTermination(int signal) {
  Logger::info(processName_ +
               " received termination signal: " + std::to_string(signal));
  cleanup();
  exit(0);
}

void BaseProcess::terminationHandler(int signal) {
  if (instance_) {
    instance_->handleTermination(signal);
  } else {
    Logger::error("Termination signal: SIG " + std::to_string(signal) +
                  " received, but no instance has been captured for PID " +
                  std::to_string(getpid()));
    exit(1);
  }
}

void BaseProcess::registerSignal(int sig, void (*handler)(int)) {
  auto result = signal(sig, handler);
  if (result == SIG_ERR) {
    std::string errorMessage =
        std::string("Failed to register signal: SIG ") + std::to_string(sig);

    if (!processName_.empty() && processName_ != "dean") {
      handleError(errorMessage.c_str());
    } else {
      perror(errorMessage.c_str());
      exit(1);
    }
  }
}

std::string BaseProcess::getProcessName(int argc, char *argv[]) {
  if (argc < 1) {
    Logger::warn("Cannot infer process name from arguments");
    return "Unknown";
  }

  std::string fullPath = argv[0];
  size_t lastSlash = fullPath.find_last_of("/");

  return (lastSlash != std::string::npos) ? fullPath.substr(lastSlash + 1)
                                          : fullPath;
}
