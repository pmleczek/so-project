#include "lifecycle.h"
#include "config.h"
#include "logger.h"
#include "processinfo.h"
#include "utils/random.h"
#include "validators.h"
#include <unistd.h>

// SECTION: Main Process
void initializeMainProcess(int argc, char *argv[]) {
  Logger::shared().log("Simulation started");
  registerMainProcess();
  configureMainProcess(argc, argv);
  SharedMemoryManager::shared().initialize(Config::shared().candidateCount);
  // TODO: Spawn examiner processes
  spawnCandidateProcesses();
  waitForExamStart();
  SharedMemoryManager::shared().destroy();
}

void registerMainProcess() {
  Logger::shared().log("Registering main process");
  pid_t mainProcessPID = getpid();
  ProcessInfo::registerProcess(mainProcessPID, ProcessType::MAIN);
}

void configureMainProcess(int argc, char *argv[]) {
  Logger::shared().log("Initializing main process");

  Logger::shared().log("Validating arguments");
  ArgValidator::validateArgs(argc, argv);

  Logger::shared().log("Generating simulation configuration");
  int candidateCount = std::stoi(argv[1]);
  int startTime = std::stoi(argv[2]);
  Config::shared().initializeFromArgs(candidateCount, startTime);
}

void spawnCandidateProcesses() {
  int count = static_cast<int>(std::floor(Config::shared().candidateCount *
                                          Config::shared().candidateRatio));
  Logger::shared().log("Spawning " + std::to_string(count) +
                       " candidate processes");
  for (int i = 0; i < count; i++) {
    SharedCandidateState candidateState = initializeCandidate();
    SharedMemoryManager &shm = SharedMemoryManager::shared();
    SharedMemoryData *data = shm.data();
    if (data) {
      data->candidates[i] = candidateState;
    } else {
      // TODO: Handle error
    }
  }
}

void waitForExamStart() {
  int startTime = Config::shared().startTime;
  Logger::shared().log("Waiting for the exam to begin for: " +
                       std::to_string(startTime) + " seconds");
  sleep(startTime);
}
// END SECTION: Main Process

// SECTION: Candidate Process
SharedCandidateState initializeCandidate() {
  pid_t candidatePID = fork();
  if (candidatePID == 0) {
    runCandidateProcess();
  } else {
    ProcessInfo::registerProcess(candidatePID, ProcessType::CANDIDATE);
    bool passedExam =
        Random::randomDouble(0, 100) > Config::shared().examFailureRate;
    bool reattempt =
        Random::randomDouble(0, 100) > Config::shared().reattemptRate;
    return SharedCandidateState{candidatePID, 0, 0, passedExam, reattempt};
  }
}

void runCandidateProcess() {
  Logger::shared().log("Hello I'm candidate with PID: " +
                       std::to_string(getpid()));
  SharedMemoryManager &shm = SharedMemoryManager::shared();
  shm.attach();
  exit(0);
}
// END SECTION: Candidate Process
