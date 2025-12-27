#include "main/MainProcess.h"

#include "candidate/CandidateFactory.h"
#include "config.h"
#include "ipc/SharedMemoryManager.h"
#include "output/Logger.h"
#include "output/ResultsWriter.h"
#include "utils/random.h"
#include "validators.h"
#include <string>
#include <unistd.h>

void MainProcess::run(int argc, char *argv[]) {
  // Step 1: Initialize the main (dean) process
  initialize(argc, argv);
  // Step 2: Spawn candidate processes
  spawnCandidateProcesses();
  // Step 3: Spawn commission processes
  // Step 4: Validate eligibility of candidates
  validateEligibility();
  // Step 5: Wait for exam start
  waitForExamStart();
  // Step 6: Run exam simulation for each candidate
  // Step 7: Publish results
  publishResults();
  // Step 8: Cleanup
  cleanup();
}

void MainProcess::initialize(int argc, char *argv[]) {
  Logger::setupLogFile();
  Logger::shared().log("Initializing main process...");

  setupConfig(argc, argv);

  if (!SharedMemoryManager::shared().initialize(
          Config::shared().candidateCount)) {
    // TODO: Error handling
    Logger::shared().log("Failed to initialize shared memory manager");
    exit(1);
  }
}

void MainProcess::setupConfig(int argc, char *argv[]) {
  Logger::shared().log("Setting up configuration for main process...");

  Logger::shared().log("Validating arguments");
  ArgValidator::validateArgs(argc, argv);

  Logger::shared().log("Generating simulation configuration");
  int candidateCount = std::stoi(argv[1]);
  int startTime = std::stoi(argv[2]);
  Config::shared().initializeFromArgs(candidateCount, startTime);
}

void MainProcess::waitForExamStart() {
  int startTime = Config::shared().startTime;
  Logger::shared().log("Waiting " + std::to_string(startTime) +
                       " seconds for exam start...");
  sleep(startTime);
}

void MainProcess::spawnCandidateProcesses() {
  Logger::shared().log("Spawning " +
                       std::to_string(Config::shared().candidateCount) +
                       " candidate processes...");

  // TODO: Extract to function
  int failedExamCount =
      (int)(Config::shared().examFailureRate * Config::shared().candidateCount);
  Logger::shared().log("Generating " + std::to_string(failedExamCount) +
                       " failed exam IDs...");
  std::unordered_set<int> failedExamIDs = Random::randomValues(
      failedExamCount, 0, Config::shared().candidateCount - 1);
  Logger::shared().log("Failed exam IDs: ");
  Logger::shared().log("Count: " + std::to_string(failedExamIDs.size()));
  for (int id : failedExamIDs) {
    Logger::shared().log(std::to_string(id));
  }

  // TODO: Extract to function
  int reattemptCount =
      (int)(Config::shared().reattemptRate * Config::shared().candidateCount);
  Logger::shared().log("Generating " + std::to_string(reattemptCount) +
                       " reattempt IDs...");
  std::unordered_set<int> reattemptIDs = Random::randomValues(
      reattemptCount, 0, Config::shared().candidateCount - 1, failedExamIDs);

  Logger::shared().log("Reattempt IDs: ");
  Logger::shared().log("Count: " + std::to_string(reattemptIDs.size()));
  for (int id : reattemptIDs) {
    Logger::shared().log(std::to_string(id));
  }

  for (int i = 0; i < Config::shared().candidateCount; i++) {
    pid_t candidatePID = fork();

    if (candidatePID == -1) {
      // TODO: Error handling
      Logger::shared().log("Failed to spawn candidate process");
      exit(1);
    }

    if (candidatePID != 0) {
      // TODO: State update?
      Logger::shared().log("Candidate process with PID " +
                           std::to_string(candidatePID) +
                           " spawned successfully");
      SharedCandidateState candidateState = CandidateFactory::create(
          candidatePID, i, failedExamIDs.find(i) == failedExamIDs.end(),
          reattemptIDs.find(i) != reattemptIDs.end());
      SharedMemoryManager::shared().data()->candidates[i] = candidateState;
    } else {
      // TODO: Add to constants
      // TODO: Pass candidate PID?
      Logger::shared().log("Spawning candidate process with PID " +
                           std::to_string(getpid()));
      execl("./so_project_candidate", "so_project_candidate", NULL);
    }
  }
}

void MainProcess::validateEligibility() {
  Logger::shared().log("Validating eligibility of candidates...");
  SharedMemoryManager &shm = SharedMemoryManager::shared();
  for (int i = 0; i < Config::shared().candidateCount; i++) {
    SharedCandidateState &candidate = shm.data()->candidates[i];
    if (!candidate.passedExam) {
      Logger::shared().log("Candidate " + std::to_string(i) + " failed exam");
      Logger::shared().log("Excluded from exam");
      candidate.finalScore = 0;
    }
  }
}

void MainProcess::publishResults() {
  Logger::shared().log("Publishing exam results...");
  ResultsWriter::writeResults();
}

void MainProcess::cleanup() {
  Logger::shared().log("Cleaning up main process...");
  SharedMemoryManager::shared().destroy();
}