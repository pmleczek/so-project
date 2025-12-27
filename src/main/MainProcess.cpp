#include "main/MainProcess.h"

#include "candidate/CandidateFactory.h"
#include "config.h"
#include "ipc/MessageQueueManager.h"
#include "ipc/SemaphoreManager.h"
#include "ipc/SharedMemoryManager.h"
#include "output/Logger.h"
#include "output/ResultsWriter.h"
#include "utils/random.h"
#include "utils/validation.h"
#include <string>
#include <unistd.h>

void MainProcess::run(int argc, char *argv[]) {
  // Step 1: Initialize the main (dean) process
  initialize(argc, argv);
  // Step 2: Spawn candidate processes
  spawnCandidateProcesses();
  // Step 3: Spawn commission processes
  spawnCommissionProcesses();
  // Step 4: Wait for exam start
  waitForExamStart();
  // Step 5: Validate eligibility of candidates
  validateEligibility();
  // Step 6: Run exam simulation for each candidate
  // Step 7: Publish results
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

  if (!SemaphoreManager::shared().initialize(Config::shared().candidateCount)) {
    // TODO: Error handling
    Logger::shared().log("Failed to initialize semaphore manager");
    exit(1);
  }

  if (!MessageQueueManager::shared().initialize()) {
    // TODO: Error handling
    Logger::shared().log("Failed to initialize message queue");
    exit(1);
  }
}

void MainProcess::setupConfig(int argc, char *argv[]) {
  Logger::shared().log("Setting up configuration for main process...");

  auto args = ArgValidator({
                               {.name = "candidateCount", .min = 1},
                               {.name = "startTime", .min = 1},
                           })
                  .validate(argc, argv);
  if (!args.has_value()) {
    // TODO: Error handling
    Logger::shared().log("Invalid arguments");
    exit(1);
  }

  Logger::shared().log("Generating simulation configuration");
  Config::shared().initializeFromArgs(args->at("candidateCount"),
                                      args->at("startTime"));
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

    bool passedExam = failedExamIDs.find(i) == failedExamIDs.end();
    bool reattempt = reattemptIDs.find(i) != reattemptIDs.end();

    if (candidatePID != 0) {
      // TODO: State update?
      Logger::shared().log("Candidate process with PID " +
                           std::to_string(candidatePID) +
                           " spawned successfully");
      SharedCandidateState candidateState =
          CandidateFactory::create(candidatePID, passedExam);
      SharedMemoryManager::shared().data()->candidates[i] = candidateState;
    } else {
      // TODO: Add to constants
      // TODO: Pass candidate PID?
      Logger::shared().log("Spawning candidate process with PID " +
                           std::to_string(getpid()));
      execl("./so_project_candidate", "so_project_candidate",
            std::to_string(i).c_str(), std::to_string(reattempt).c_str(), NULL);
    }
  }
}

void MainProcess::spawnCommissionProcesses() {
  Logger::shared().log("Spawning commission processes...");

  // Spawn comission A
  pid_t aPID = fork();
  if (aPID == -1) {
    // TODO: Error handling
    Logger::shared().log("Failed to spawn commission A process");
    exit(1);
  }

  if (aPID == 0) {
    // TODO: Add to constants
    Logger::shared().log("Spawning commission A process with PID " +
                         std::to_string(aPID));
    execl("./so_project_commission", "so_project_commission", "A", NULL);
  }
}

void MainProcess::validateEligibility() {
  Logger::shared().log("Validating eligibility of candidates...");
  SharedMemoryManager &shm = SharedMemoryManager::shared();
  SemaphoreManager &sm = SemaphoreManager::shared();
  MessageQueueManager &mq = MessageQueueManager::shared();

  for (int i = 0; i < Config::shared().candidateCount; i++) {
    SharedCandidateState &candidate = shm.data()->candidates[i];

    if (!candidate.passedExam) {
      Logger::shared().log("Candidate " + std::to_string(i) +
                           " failed exam. Excluding from exam...");
      candidate.finalScore = 0;
      candidate.status = INELIGIBLE;
    }

    mq.send(ELIGIBILITY_REQUEST, i, 0, candidate.passedExam ? "1" : "0");
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