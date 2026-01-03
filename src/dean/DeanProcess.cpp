#include "dean/DeanProcess.h"

#include "common/ipc/SemaphoreManager.h"
#include "common/ipc/SharedMemoryManager.h"
#include "common/output/Logger.h"
#include "common/output/ResultsWriter.h"
#include "common/process/ProcessRegistry.h"
#include "common/utils/Memory.h"
#include "common/utils/Random.h"
#include "common/utils/Time.h"
#include <regex>
#include <signal.h>
#include <sys/resource.h>
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
  /* Create shared memory and initialize its state */
  SharedMemoryManager::initialize(config.candidateCount);
  SharedMemoryManager::data()->candidateCount = config.candidateCount;
  for (int i = 0; i < 3; i++) {
    Memory::resetSeat('A', i);
    Memory::resetSeat('B', i);
  }

  /* Initialize mutex for shared memory */
  try {
    Memory::initializeMutex();
  } catch (const std::exception &e) {
    std::string errorMessage =
        "Failed to initialize mutex for shared memory: " +
        std::string(e.what());
    handleError(errorMessage.c_str());
  }

  /* Create semaphores for commissions */
  SemaphoreManager::create("commissionA", 0);
  SemaphoreManager::create("commissionB", 0);
}

/**
 * Sets up signal handlers for the dean process.
 */
void DeanProcess::setupSignalHandlers() {
  /* SECTION: Termination signals */
  auto result = signal(SIGINT, DeanProcess::terminationHandler);
  if (result == SIG_ERR) {
    handleError("Failed to set up SIGINT handler");
  }

  result = signal(SIGTERM, DeanProcess::terminationHandler);
  if (result == SIG_ERR) {
    handleError("Failed to set up SIGTERM handler");
  }

  result = signal(SIGQUIT, DeanProcess::terminationHandler);
  if (result == SIG_ERR) {
    handleError("Failed to set up SIGQUIT handler");
  }
  /* END SECTION: Termination signals */

  /* SECTION: Evacuation signal */
  result = signal(SIGUSR1, DeanProcess::evacuationHandler);
  if (result == SIG_ERR) {
    handleError("Failed to set up SIGUSR1 handler");
  }
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
  int result = sleep(sleepTime);
  if (result < 0) {
    handleError("Failed in sleep() call to wait for exam start");
  }
}

/**
 * Starts the exam and waits for it to end.
 */
void DeanProcess::start() {
  Logger::info("DeanProcess::start()");
  SharedMemoryManager::data()->examStarted = true;

  while (SharedMemoryManager::data()->examStarted) {
    usleep(1000000);
  }

  Logger::info("Exam ended");
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
      execlp("./candidate", "./candidate", std::to_string(i).c_str(), NULL);
      std::string errorMessage =
          "Failed in execlp() call for candidate " + std::to_string(i);
      handleError(errorMessage.c_str());
    } else {
      SharedMemoryManager::data()->candidates[i].pid = candidatePid;
      SharedMemoryManager::data()->candidates[i].practicalScore = -1.0;
      SharedMemoryManager::data()->candidates[i].finalScore = -1.0;

      failed = failedExamIndices.find(i) != failedExamIndices.end();
      retake = retakeExamIndices.find(i) != retakeExamIndices.end();

      if (failed) {
        SharedMemoryManager::data()->candidates[i].status = NotEligible;
      } else {
        SharedMemoryManager::data()->candidates[i].status = PendingCommissionA;
      }

      if (retake) {
        double theoreticalScore = Random::randomDouble(30.0, 100.0);
        SharedMemoryManager::data()->candidates[i].theoreticalScore =
            theoreticalScore;
        retaking++;
      } else {
        SharedMemoryManager::data()->candidates[i].theoreticalScore = -1.0;
      }
    }

    // TODO: Investigate
    // For stability reasons
    // int result = usleep(10000);
    // if (result < 0) {
    //   std::string errorMessage = "Failed in usleep() call for candidate " +
    //                              std::to_string(i);
    //   handleError(errorMessage.c_str());
    // }
  }
}

/**
 * Verifies candidates eligibility for the exam.
 */
void DeanProcess::verifyCandidates() {
  Logger::info("DeanProcess::verifyCandidates()");

  int rejected = 0;
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

  int commissonACount = config.candidateCount - rejected - retaking;
  SharedMemoryManager::data()->commissionACandidateCount = commissonACount;
  SharedMemoryManager::data()->commissionBCandidateCount = commissonACount;
}

/**
 * Cleans up the dean process.
 */
void DeanProcess::cleanup() {
  Logger::info("DeanProcess::cleanup()");

  SharedMemoryManager::destroy();
  SemaphoreManager::unlink("/commissionA");
  SemaphoreManager::unlink("/commissionB");

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
    instance_->cleanup();
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
    instance_->cleanup();
    instance_ = nullptr;
  } else {
    Logger::error("No instance found for evacuation handler");
    exit(1);
  }

  exit(0);
}
