#include "candidate/CandidateProcess.h"

#include "common/ipc/MutexWrapper.h"
#include "common/ipc/SharedMemoryManager.h"
#include "common/output/Logger.h"
#include "common/process/ProcessRegistry.h"
#include "common/utils/Misc.h"
#include <signal.h>

/**
 * Constructor for the candidate process.
 *
 * @param argc The number of arguments passed to the program.
 * @param argv The arguments passed to the program.
 */
CandidateProcess::CandidateProcess(int argc, char *argv[])
    : BaseProcess(argc, argv, false) {
  try {
    validateArguments(argc, argv);
  } catch (const std::exception &e) {
    std::string errorMessage =
        "Failed to validate arguments: \n\t" + std::string(e.what());
    handleError(errorMessage.c_str());
    exit(1);
  }

  setupSignalHandlers();

  try {
    initialize();
  } catch (const std::exception &e) {
    std::string errorMessage = "Failed to initialize process " + processName_ +
                               "\n: " + std::string(e.what());
    handleError(errorMessage.c_str());
    exit(1);
  }
}

/**
 * Validates the arguments passed to the program.
 *
 * @param argc The number of arguments passed to the program.
 * @param argv The arguments passed to the program.
 * @throws std::invalid_argument If the arguments are invalid.
 */
void CandidateProcess::validateArguments(int argc, char *argv[]) {
  /* Validate argument count */
  /* Candidate index */
  /* + 5 times for commission A */
  /* + 3 times for commission B */
  if (argc != 10) {
    throw std::invalid_argument("Invalid number of arguments. Usage: "
                                "./candidate <index> <timesA> <timesB>");
  }

  /* Validate candidate index */
  /* Expected format: integer */
  /* Expected value: n >= 0 */
  int candidateIndex = std::stoi(argv[1]);
  if (candidateIndex < 0) {
    throw std::invalid_argument("Candidate index must be non-negative");
  }
  index = candidateIndex;
  Logger::setProcessPrefix("Candidate (id=" + std::to_string(index) + ")");

  /* Validate times for commission A */
  /* Expected format: double[] */
  /* Expected value: t[i] > 0.0 */
  for (int i = 0; i < 5; i++) {
    timesA[i] = std::stod(argv[2 + i]);
    if (timesA[i] <= 0.0) {
      throw std::invalid_argument("Times A must be positive");
    }
  }

  /* Validate times for commission B */
  /* Expected format: double[] */
  /* Expected value: t[i] > 0.0 */
  for (int i = 0; i < 3; i++) {
    timesB[i] = std::stod(argv[7 + i]);
    if (timesB[i] <= 0.0) {
      throw std::invalid_argument("Times B must be positive");
    }
  }
}

/**
 * Initializes the candidate process.
 */
void CandidateProcess::initialize() { SharedMemoryManager::attach(); }

/**
 * Sets up the signal handlers for the candidate process.
 */
void CandidateProcess::setupSignalHandlers() {
  registerSignal(SIGUSR2, rejectionHandler);
  registerSignal(SIGTERM, terminationHandler);
}

/**
 * Handles an error by sending a SIGTERM to the dean process and exiting the
 * process with status 1.
 *
 * @param message The message to display.
 */
void CandidateProcess::handleError(const char *message) {
  /* Include both: the passed message and the errno message */
  if (message != nullptr) {
    perror(message);
    perror(nullptr);
  } else {
    perror(message);
  }

  int result = kill(getppid(), SIGTERM);
  if (result < 0) {
    perror("kill() failed to send SIGTERM to dean process");
  }

  cleanup();

  exit(1);
}

/**
 * Waits for the exam to start by checking the examStarted flag in the shared
 * memory.
 */
void CandidateProcess::waitForExamStart() {
  try {
    Logger::info("CandidateProcess::waitForExamStart()");
    pthread_mutex_t *examStateMutex =
        &SharedMemoryManager::data()->examStateMutex;

    while (true) {
      MutexWrapper::lock(examStateMutex);
      if (SharedMemoryManager::data()->examStarted) {
        MutexWrapper::unlock(examStateMutex);
        break;
      }
      MutexWrapper::unlock(examStateMutex);

      Misc::safeSleep(1);
    }
  } catch (const std::exception &e) {
    std::string errorMessage =
        "Failed to wait for exam start: " + std::string(e.what());
    handleError(errorMessage.c_str());
  }
}

/**
 * Gets a seat for the given commission.
 *
 * @param commission The commission to get a seat for.
 */
void CandidateProcess::getCommissionSeat(char commission) {
  /* Reset seat for commission B */
  if (commission == 'B') {
    seat = -1;
  }

  try {
    sem_t *semaphore = SemaphoreManager::open(
        (commission == 'A') ? "commissionA" : "commissionB");

    while (seat == -1) {
      SemaphoreManager::wait(semaphore);
      seat = findCommissionSeat(commission);

      if (seat == -1) {
        SemaphoreManager::post(semaphore);
        Misc::safeUSleep(10000);
      }
    }

    SemaphoreManager::close(semaphore);
  } catch (const std::exception &e) {
    std::string errorMessage =
        "Failed to get commission seat: " + std::string(e.what());
    handleError(errorMessage.c_str());
  }
}

/**
 * Finds a seat for the given commission.
 *
 * @param commission The commission to find a seat for.
 * @return The seat number, or -1 if no seat is found.
 */
int CandidateProcess::findCommissionSeat(char commission) {
  pthread_mutex_t *comissionMutex = nullptr;
  if (commission == 'A') {
    comissionMutex = &SharedMemoryManager::data()->commissionAMutex;
  } else {
    comissionMutex = &SharedMemoryManager::data()->commissionBMutex;
  }

  try {
    MutexWrapper::lock(comissionMutex);
    CommissionInfo *commissionInfo =
        commission == 'A' ? &SharedMemoryManager::data()->commissionA
                          : &SharedMemoryManager::data()->commissionB;

    for (int i = 0; i < 3; i++) {
      if (commissionInfo->seats[i].pid == -1) {
        commissionInfo->seats[i] = {
            .pid = getpid(), .questionsCount = 0, .answered = false};
        MutexWrapper::unlock(comissionMutex);
        return i;
      }
    }

    MutexWrapper::unlock(comissionMutex);
  } catch (const std::exception &e) {
    std::string errorMessage =
        "Failed to find commission seat: " + std::string(e.what());
    handleError(errorMessage.c_str());
  }

  return -1;
}

/**
 * Waits for the questions to be available by checking the questionsCount in
 * the shared memory.
 */
void CandidateProcess::waitForQuestions(char commission) {
  pthread_mutex_t *comissionMutex = nullptr;
  if (commission == 'A') {
    comissionMutex = &SharedMemoryManager::data()->commissionAMutex;
  } else {
    comissionMutex = &SharedMemoryManager::data()->commissionBMutex;
  }

  try {
    Logger::info("CandidateProcess::waitForQuestions()");
    Logger::info("Candidate process with pid " + std::to_string(getpid()) +
                 " waiting for questions from commission " + commission);

    while (true) {
      MutexWrapper::lock(comissionMutex);
      if ((commission == 'A' && SharedMemoryManager::data()
                                        ->commissionA.seats[seat]
                                        .questionsCount == (1 << 5) - 1) ||
          (commission == 'B' && SharedMemoryManager::data()
                                        ->commissionB.seats[seat]
                                        .questionsCount == (1 << 3) - 1)) {
        MutexWrapper::unlock(comissionMutex);
        break;
      }
      MutexWrapper::unlock(comissionMutex);

      Misc::safeSleep(1);
    }
  } catch (const std::exception &e) {
    std::string errorMessage =
        "Failed to wait for questions: " + std::string(e.what());
    handleError(errorMessage.c_str());
  }
}

/**
 * Prepares the answers by setting the answered flag in the shared memory.
 */
void CandidateProcess::prepareAnswers(char commission) {
  double sleepTime = 0.0;
  if (commission == 'A') {
    for (int i = 0; i < 5; i++) {
      sleepTime += timesA[i];
    }
  } else {
    for (int i = 0; i < 3; i++) {
      sleepTime += timesB[i];
    }
  }

  try {
    Misc::safeUSleep(sleepTime * 1000000);

    pthread_mutex_t *comissionMutex = nullptr;
    if (commission == 'A') {
      comissionMutex = &SharedMemoryManager::data()->commissionAMutex;
    } else {
      comissionMutex = &SharedMemoryManager::data()->commissionBMutex;
    }

    MutexWrapper::lock(comissionMutex);
    if (commission == 'A') {
      SharedMemoryManager::data()->commissionA.seats[seat].answered = true;
    } else {
      SharedMemoryManager::data()->commissionB.seats[seat].answered = true;
    }
    MutexWrapper::unlock(comissionMutex);
  } catch (const std::exception &e) {
    std::string errorMessage =
        "Failed to sleep in prepareAnswers: " + std::string(e.what());
    handleError(errorMessage.c_str());
  }

  Logger::info("Candidate process with pid " + std::to_string(getpid()) +
               " answered questions");
}

/**
 * Waits for the grading to be available by checking the theoreticalScore or
 * practicalScore in the shared memory.
 */
void CandidateProcess::waitForGrading(char commission) {
  pthread_mutex_t *candidatesMutex =
      &SharedMemoryManager::data()->candidateMutex;

  try {
    while (true) {
      MutexWrapper::lock(candidatesMutex);
      if (commission == 'A') {
        if (SharedMemoryManager::data()->candidates[index].theoreticalScore >=
            0) {
          MutexWrapper::unlock(candidatesMutex);
          break;
        }
      } else {
        if (SharedMemoryManager::data()->candidates[index].practicalScore >=
            0) {
          MutexWrapper::unlock(candidatesMutex);
          break;
        }
      }
      MutexWrapper::unlock(candidatesMutex);

      Misc::safeSleep(1);
    }
  } catch (const std::exception &e) {
    std::string errorMessage =
        "Failed to wait for grading: " + std::string(e.what());
    handleError(errorMessage.c_str());
  }
}

/**
 * Exits the exam if the candidate failed the commission A.
 */
void CandidateProcess::maybeExitExam() {
  pthread_mutex_t *candidatesMutex =
      &SharedMemoryManager::data()->candidateMutex;

  MutexWrapper::lock(candidatesMutex);
  if (SharedMemoryManager::data()->candidates[index].theoreticalScore < 30) {
    MutexWrapper::unlock(candidatesMutex);
    Logger::info("Candidate process with pid " + std::to_string(getpid()) +
                 " failed to pass the exam");
    exit(0);
  }
  MutexWrapper::unlock(candidatesMutex);
}

/**
 * Cleans up the candidate process.
 */
void CandidateProcess::cleanup() {
  Logger::info("CandidateProcess::cleanup()");

  try {
    SharedMemoryManager::detach();
  } catch (const std::exception &e) {
    std::string errorMessage =
        "Failed to detach from shared memory: " + std::string(e.what());
    perror(errorMessage.c_str());
  }

  Logger::info("Candidate process with pid " + std::to_string(getpid()) +
               " exiting with status 0");
  ProcessRegistry::unregister(getpid());
}

/**
 * Handles the rejection signal by cleaning up the candidate process.
 *
 * @param signal The signal that caused the rejection.
 */
void CandidateProcess::rejectionHandler(int signal) {
  Logger::info("CandidateProcess::rejectionHandler()");
  Logger::info("Candidate process with pid " + std::to_string(getpid()) +
               " exiting with status 0");

  if (instance_) {
    static_cast<CandidateProcess *>(instance_)->cleanup();
    instance_ = nullptr;
  }

  exit(0);
}

/**
 * Handles the termination signal by cleaning up the candidate process.
 *
 * @param signal The signal that caused the termination.
 */
void CandidateProcess::terminationHandler(int signal) {
  Logger::info("CandidateProcess::terminationHandler()");
  Logger::info("Candidate process with pid " + std::to_string(getpid()) +
               " exiting with status 0 (terminated)");

  if (instance_) {
    static_cast<CandidateProcess *>(instance_)->cleanup();
    instance_ = nullptr;
  }

  exit(0);
}
