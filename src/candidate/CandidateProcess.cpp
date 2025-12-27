#include "candidate/CandidateProcess.h"

#include "ipc/SharedMemoryManager.h"
#include "output/Logger.h"

void CandidateProcess::run() {
  // Step 1: Initialize candidate process
  initialize();
  // Step : Cleanup
  cleanup();
  // Step : Exit
  exit(0);
}

void CandidateProcess::initialize() {
  Logger::shared().log("Initializing candidate process...");
  SharedMemoryManager::shared().attach();
}

void CandidateProcess::cleanup() {
  Logger::shared().log("Cleaning up candidate process...");
  SharedMemoryManager::shared().detach();
}
