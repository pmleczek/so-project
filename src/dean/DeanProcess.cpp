#include "dean/DeanProcess.h"

#include "common/ipc/MutexWrapper.h"
#include "common/ipc/SemaphoreManager.h"
#include "common/ipc/SharedMemoryManager.h"
#include "common/output/Logger.h"
#include "common/output/ResultsWriter.h"
#include "common/process/ProcessRegistry.h"
#include "common/utils/Memory.h"
#include "common/utils/Misc.h"
#include "common/utils/Random.h"
#include "common/utils/Time.h"
#include <cerrno>
#include <regex>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * Constructor for the dean process.
 *
 * @param argc The number of arguments passed to the program.
 * @param argv The arguments passed to the program.
 */
DeanProcess::DeanProcess(int argc, char *argv[])
    : BaseProcess(argc, argv, true), config() {
  try {
    validateArguments(argc, argv);
  } catch (const std::exception &e) {
    std::string errorMessage =
        "Failed to validate arguments: \n\t" + std::string(e.what());
    perror(errorMessage.c_str());
    exit(1);
  }

  setupSignalHandlers();

  try {
    initialize();
  } catch (const std::exception &e) {
    std::string errorMessage = "Failed to initialize process " + processName_ +
                               "\n: " + std::string(e.what());
    perror(errorMessage.c_str());
    exit(1);
  }
}

/**
 * Validates the arguments passed to the program and initializes the
 * configuration.
 *
 * @param argc The number of arguments passed to the program.
 * @param argv The arguments passed to the program.
 * @throws std::invalid_argument If the arguments are invalid.
 */
void DeanProcess::validateArguments(int argc, char *argv[]) {
  /* Validate argument count */
  if (argc != 3) {
    throw std::invalid_argument("Invalid number of arguments. Usage: ./dean "
                                "<place count> <start time>");
  }

  /* Get maximum possible process count */
  int MAX_CANDIDATE_COUNT = -1;
  struct rlimit rl;
  if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
    MAX_CANDIDATE_COUNT = rl.rlim_cur;
  } else {
    MAX_CANDIDATE_COUNT = sysconf(_SC_CHILD_MAX);
    if (MAX_CANDIDATE_COUNT == -1) {
      handleError("Failed to infer MAX_CANDIDATE_COUNT");
    }
  }

  /* Limit possible process count to 90% of available processes */
  MAX_CANDIDATE_COUNT = MAX_CANDIDATE_COUNT * 0.9;
  /* Assume maximum candidate to place ratio (random value between 9.5 and 10.5)
   */
  MAX_CANDIDATE_COUNT = MAX_CANDIDATE_COUNT / 10.5;

  /* Validate place count */
  /* Expected format: integer */
  /* Expected value: 0 < n <= MAX_CANDIDATE_COUNT */
  int placeCount = std::stoi(argv[1]);
  if (placeCount <= 0 || placeCount > MAX_CANDIDATE_COUNT) {
    throw std::invalid_argument("Invalid place count. Expected: 0 < n <= " +
                                std::to_string(MAX_CANDIDATE_COUNT));
  }

  /* Validate start time */
  /* Expected format: HH:MM (24 hrs) */
  /* Expected value: correct time, >= current time */
  std::regex timeRegex("^([01][0-9]|2[0-3]):[0-5][0-9]$");
  if (!std::regex_match(argv[2], timeRegex)) {
    throw std::invalid_argument("Start time is invalid or has an invalid "
                                "format. Expected format: HH:MM (24 hrs)");
  }

  /* Add 59 seconds to make the start time as late as possible */
  int seconds = Time::seconds(argv[2]) + 59;
  if (seconds < Time::now()) {
    throw std::invalid_argument("Start time is in the past");
  }

  /* Initialize the dean proces configuration */
  config = DeanConfig(placeCount, seconds);
}

/**
 * Initializes the dean process and IPC mechanisms.
 */
void DeanProcess::initialize() {
  try {
    /* Set up logger prefix  */
    Logger::setProcessPrefix("Dean (pid=" + std::to_string(pid_) + ")");

    /* Create shared memory and initialize its state */
    /* No mutex because dean is only process that writes to shared memory as of
     * now */
    SharedMemoryManager::initialize(config.candidateCount);
    SharedMemoryManager::data()->candidateCount = config.candidateCount;
    for (int i = 0; i < 3; i++) {
      Memory::resetSeat('A', i);
      Memory::resetSeat('B', i);
    }

    /* Initialize mutexes*/
    Memory::initializeMutex();

    /* Create semaphores for commissions */
    SemaphoreManager::create("commissionA", 0);
    SemaphoreManager::create("commissionB", 0);
    SemaphoreManager::create("logger", 1);
  } catch (const std::exception &e) {
    std::string errorMessage =
        "Failed to initialize mutex for shared memory: " +
        std::string(e.what());
    handleError(errorMessage.c_str());
  }
}

/**
 * Sets up signal handlers for the dean process.
 */
void DeanProcess::setupSignalHandlers() {
  /* SECTION: Termination signals */
  registerSignal(SIGINT, DeanProcess::terminationHandler);
  registerSignal(SIGTERM, DeanProcess::terminationHandler);
  registerSignal(SIGQUIT, DeanProcess::terminationHandler);
  /* END SECTION: Termination signals */

  /* SECTION: Evacuation signal */
  registerSignal(SIGUSR1, DeanProcess::evacuationHandler);
  /* END SECTION: Evacuation signal */
}

/**
 * Handles a fatal error.
 *
 * @param message The error message.
 */
void DeanProcess::handleError(const char *message) {
  /* Include both: the passed message and the errno message */
  if (message != nullptr) {
    perror(message);
    perror(nullptr);
  } else {
    perror(message);
  }

  /* Propagate the termination signal to all child processes and cleanup */
  ProcessRegistry::propagateSignal(SIGTERM);
  cleanup();

  exit(1);
}

/**
 * Waits for the exam to start.
 */
void DeanProcess::waitForExamStart() {
  int sleepTime = config.startTime - Time::now();

  /* Sleep for at least 15 seconds to avoid race conditions */
  sleepTime = std::max(sleepTime, 15);

  Logger::info("Waiting for exam start for " + std::to_string(sleepTime) +
               " seconds");
  try {
    Misc::safeSleep(sleepTime);
  } catch (const std::exception &e) {
    std::string errorMessage =
        "Failed to sleep in waitForExamStart: " + std::string(e.what());
    handleError(errorMessage.c_str());
  }

  Logger::info("Exam has started");
}

/**
 * Starts the exam and waits for it to end.
 */
void DeanProcess::start() {
  pthread_mutex_t *examStateMutex =
      &SharedMemoryManager::data()->examStateMutex;

  try {
    MutexWrapper::lock(examStateMutex);
    SharedMemoryManager::data()->examStarted = true;
    MutexWrapper::unlock(examStateMutex);

    int commissionAPID = SharedMemoryManager::data()->commissionAPID;
    if (commissionAPID != -1) {
      int status;
      if (waitpid(commissionAPID, &status, 0) == -1 && errno != ECHILD) {
        Logger::warn("Failed to wait for commission A: " +
                     std::string(strerror(errno)));
      } else {
        Logger::info("Commission A process finished");
      }
    }

    int commissionBPID = SharedMemoryManager::data()->commissionBID;
    if (commissionBPID != -1) {
      int status;
      if (waitpid(commissionBPID, &status, 0) == -1 && errno != ECHILD) {
        Logger::warn("Failed to wait for commission B: " +
                     std::string(strerror(errno)));
      } else {
        Logger::info("Commission B process finished");
      }
    }
  } catch (const std::exception &e) {
    std::string errorMessage =
        "Failed to sleep in start: " + std::string(e.what());
    handleError(errorMessage.c_str());
  }

  Logger::info("Exam ended. Publishing results...");
  ResultsWriter::publishResults(false);
}

/**
 * Spawns commission processes.
 */
void DeanProcess::spawnComissions() {
  /* Comission A */
  pid_t pidA = fork();
  if (pidA < 0) {
    handleError("Failed in fork() call for commission A");
  }

  if (pidA == 0) {
    execlp("./commission", "./commission", "A", NULL);
    handleError("Failed in execlp() call for commission A");
  }

  ProcessRegistry::registerCommission(pidA, 'A');

  /* Comission B */
  pid_t pidB = fork();
  if (pidB < 0) {
    handleError("Failed in fork() call for commission B");
  }

  if (pidB == 0) {
    execlp("./commission", "./commission", "B", NULL);
    handleError("Failed in execlp() call for commission B");
  }

  ProcessRegistry::registerCommission(pidB, 'B');
}

/**
 * Generates random indices of candidates that failed the exam.
 */
std::unordered_set<int> DeanProcess::getFailedExamIndices() {
  auto indices =
      Random::randomInts(config.failedExamCount, 0, config.candidateCount - 1);

  Logger::info("Failed exam indices (" + std::to_string(indices.size()) +
               "): ");
  for (int index : indices) {
    Logger::info("- " + std::to_string(index));
  }

  return indices;
}

/**
 * Generates random indices of candidates that are retaking the exam.
 */
std::unordered_set<int>
DeanProcess::getRetakeExamIndices(std::unordered_set<int> excludedIndices) {
  auto indices = Random::randomInts(config.retakeExamCount, 0,
                                    config.candidateCount - 1, excludedIndices);

  Logger::info("Retake exam indices (" + std::to_string(indices.size()) +
               "): ");
  for (int index : indices) {
    Logger::info("- " + std::to_string(index));
  }

  return indices;
}

/**
 * Spawns candidate processes.
 */
void DeanProcess::spawnCandidates() {
  Logger::info("Spawning " + std::to_string(config.candidateCount) +
               " candidates");

  std::unordered_set<int> failedExamIndices = getFailedExamIndices();
  std::unordered_set<int> retakeExamIndices =
      getRetakeExamIndices(failedExamIndices);

  pthread_mutex_t *candidatesMutex =
      &SharedMemoryManager::data()->candidateMutex;

  pid_t candidatePid;
  bool failed, retake;

  for (int i = 0; i < config.candidateCount; i++) {
    candidatePid = fork();
    if (candidatePid < 0) {
      std::string errorMessage =
          "Failed in fork() call for candidate " + std::to_string(i);
      handleError(errorMessage.c_str());
    }

    if (candidatePid == 0) {
      execlp("./candidate", "./candidate", std::to_string(i).c_str(),
             std::to_string(config.timesA[0]).c_str(),
             std::to_string(config.timesA[1]).c_str(),
             std::to_string(config.timesA[2]).c_str(),
             std::to_string(config.timesA[3]).c_str(),
             std::to_string(config.timesA[4]).c_str(),
             std::to_string(config.timesB[0]).c_str(),
             std::to_string(config.timesB[1]).c_str(),
             std::to_string(config.timesB[2]).c_str(), NULL);
      std::string errorMessage =
          "Failed in execlp() call for candidate " + std::to_string(i);
      handleError(errorMessage.c_str());
    } else {
      try {
        MutexWrapper::lock(childPidsMutex);
        childPids.push_back(candidatePid);
        MutexWrapper::unlock(childPidsMutex);

        MutexWrapper::lock(candidatesMutex);

        SharedMemoryManager::data()->candidates[i].pid = candidatePid;
        SharedMemoryManager::data()->candidates[i].practicalScore = -1.0;
        SharedMemoryManager::data()->candidates[i].finalScore = -1.0;

        failed = failedExamIndices.find(i) != failedExamIndices.end();
        retake = retakeExamIndices.find(i) != retakeExamIndices.end();

        if (failed) {
          SharedMemoryManager::data()->candidates[i].status = NotEligible;
        } else {
          SharedMemoryManager::data()->candidates[i].status =
              PendingCommissionA;
        }

        if (retake) {
          double theoreticalScore = Random::randomDouble(30.0, 100.0);
          SharedMemoryManager::data()->candidates[i].theoreticalScore =
              theoreticalScore;
          retaking++;
        } else {
          SharedMemoryManager::data()->candidates[i].theoreticalScore = -1.0;
        }

        MutexWrapper::unlock(candidatesMutex);
      } catch (const std::exception &e) {
        std::string errorMessage = "Failed to spawn candidate " +
                                   std::to_string(i) + ": " +
                                   std::string(e.what());
        handleError(errorMessage.c_str());
      }
    }
  }
}

/**
 * Verifies candidates eligibility for the exam.
 */
void DeanProcess::verifyCandidates() {
  pthread_mutex_t *candidatesMutex =
      &SharedMemoryManager::data()->candidateMutex;

  int rejected = 0;

  try {
    MutexWrapper::lock(candidatesMutex);

    for (int i = 0; i < config.candidateCount; i++) {
      if (SharedMemoryManager::data()->candidates[i].status == NotEligible) {
        int result =
            kill(SharedMemoryManager::data()->candidates[i].pid, SIGUSR2);
        if (result < 0) {
          std::string errorMessage = "Failed to send SIGUSR2 to candidate " +
                                     std::to_string(i) + ": " +
                                     std::strerror(errno);
          handleError(errorMessage.c_str());
        }
        rejected++;
      }
    }

    MutexWrapper::unlock(candidatesMutex);

  } catch (const std::exception &e) {
    std::string errorMessage =
        "Failed to verify candidates " + std::string(e.what());
    handleError(errorMessage.c_str());
  }

  pthread_mutex_t *examStateMutex =
      &SharedMemoryManager::data()->examStateMutex;
  MutexWrapper::lock(examStateMutex);

  int commissionBCount = config.candidateCount - rejected;
  SharedMemoryManager::data()->commissionACandidateCount =
      commissionBCount - retaking;
  SharedMemoryManager::data()->commissionBCandidateCount = commissionBCount;

  MutexWrapper::unlock(examStateMutex);
}

/**
 * Cleans up the dean process.
 */
void DeanProcess::cleanup() {
  Logger::info("DeanProcess::cleanup()");

  try {
    SharedMemoryManager::destroy();
    SemaphoreManager::unlink("/commissionA");
    SemaphoreManager::unlink("/commissionB");
    SemaphoreManager::unlink("/logger");
  } catch (const std::exception &e) {
    std::string errorMessage =
        "Failed to cleanup the dean process: " + std::string(e.what());
    perror(errorMessage.c_str());
  }

  Logger::info("Dean process exiting with status 0");
}

/**
 * Handles the evacuation signal.
 */
void DeanProcess::evacuationHandler(int signal) {
  Logger::info("DeanProcess::evacuationHandler()");
  Logger::info("Evacuation signal received");

  ProcessRegistry::propagateSignal(SIGTERM);
  ResultsWriter::publishResults(true);

  if (instance_) {
    static_cast<DeanProcess *>(instance_)->cleanup();
    instance_ = nullptr;
  } else {
    Logger::error("No instance found for evacuation handler");
    exit(1);
  }

  exit(0);
}

/**
 * Handles the termination signal.
 */
void DeanProcess::terminationHandler(int signal) {
  Logger::info("DeanProcess::terminationHandler()");
  Logger::info("Termination signal: SIG " + std::to_string(signal) +
               " received");

  ProcessRegistry::propagateSignal(SIGTERM);

  if (instance_) {
    static_cast<DeanProcess *>(instance_)->cleanup();
    instance_ = nullptr;
  } else {
    Logger::error("No instance found for evacuation handler");
    exit(1);
  }

  exit(0);
}

/**
 * Cleans up the child processes
 */
void *DeanProcess::cleanupThreadFunction(void *arg) {
  DeanProcess* self = static_cast<DeanProcess*>(arg);
  while (self->reaperRunning) {
    int status;
    try {
      pid_t waitedPid = waitpid(-1, &status, WNOHANG);
      if (waitedPid > 0) {
        pthread_mutex_lock(&self->childPidsMutex);
        self->childPids.erase(std::remove(self->childPids.begin(),
                                          self->childPids.end(), waitedPid),
                              self->childPids.end());
        pthread_mutex_unlock(&self->childPidsMutex);
        Logger::info("Cleanup thread: cleaned up process " +
                     std::to_string(waitedPid));
      } else if (waitedPid == 0) {
        safeUSleep(100000);
      } else if (errno == ECHILD) {
        safeUSleep(100000);
      }
    }
  }
}
