#include "main/MainProcess.h"

#include "config.h"
#include "ipc/SharedMemoryManager.h"
#include "output/Logger.h"
#include <unistd.h>

void MainProcess::run(int argc, char *argv[]) {
  // Step 1: Initialize the main (dean) process
  initialize(argc, argv);
  // Step 2: Spawn candidate processes
  // Step 3: Spawn commission processes
  // Step 4: Validate eligibility of candidates
  // Step 5: Wait for exam start
  waitForExamStart();
  // Step 6: Run exam simulation for each candidate
  // Step 7: Publish results
  // Step 8: Cleanup
  cleanup();
}

void MainProcess::initialize(int argc, char *argv[]) {
  Logger::setupLogFile();
  Logger::shared().log("Initializing main process...");
  setupConfig(argc, argv);
  SharedMemoryManager::shared().initialize(Config::shared().candidateCount);
}

void MainProcess::setupConfig(int argc, char *argv[]) {
  Logger::shared().log("Setting up configuration for main process...");
}

void MainProcess::waitForExamStart() {
  int startTime = Config::shared().startTime;
  Logger::shared().log("Waiting " + std::to_string(startTime) +
                       " seconds for exam start...");
  sleep(startTime);
}

void MainProcess::cleanup() {
  Logger::shared().log("Cleaning up main process...");
  SharedMemoryManager::shared().destroy();
}