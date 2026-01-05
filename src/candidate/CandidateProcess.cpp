#include "candidate/CandidateProcess.h"

#include "common/ipc/SharedMemoryManager.h"
#include "common/output/Logger.h"
#include "common/process/ProcessRegistry.h"
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
  auto result = signal(SIGUSR2, rejectionHandler);
  if (result == SIG_ERR) {
    perror("signal() failed to register SIGUSR2 handler");
    ProcessRegistry::unregister(getpid());
    int result = kill(getppid(), SIGTERM);
    if (result < 0) {
      perror("kill() failed to send SIGTERM to dean process");
    }
    exit(1);
  }

  result = signal(SIGTERM, terminationHandler);
  if (result == SIG_ERR) {
    perror("signal() failed to register SIGTERM handler");
    ProcessRegistry::unregister(getpid());
    int result = kill(getppid(), SIGTERM);
    if (result < 0) {
      perror("kill() failed to send SIGTERM to dean process");
    }
    exit(1);
  }
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
  Logger::info("CandidateProcess::waitForExamStart()");
  while (!SharedMemoryManager::data()->examStarted) {
    int result = sleep(1);
    if (result < 0) {
      handleError("Failed to sleep in waitForExamStart()");
    }
  }
}

/**
 * Gets a commission A seat by checking the semaphore and the shared memory.
 */
void CandidateProcess::getCommissionASeat() {
  Logger::info("CandidateProcess::getCommissionASeat()");

  if (SharedMemoryManager::data()->candidates[index].theoreticalScore >= 30.0) {
    Logger::info("Candidate process with pid " + std::to_string(getpid()) +
                 " doesn't have to take commission A exam");
    return;
  }

  semaphoreA = SemaphoreManager::open("commissionA");

  Logger::info("Candidate process with pid " + std::to_string(getpid()) +
               " waiting for commission A seat");

  while (seat == -1) {
    SemaphoreManager::wait(semaphoreA);
    seat = findCommissionASeat();
    if (seat == -1) {
      SemaphoreManager::post(semaphoreA);
      int result = usleep(10000);
      if (result < 0) {
        handleError("Failed to sleep in getCommissionASeat()");
      }
    }
  }
}

/**
 * Finds a commission A seat by checking the shared memory.
 */
int CandidateProcess::findCommissionASeat() {
  // TODO: Use mutex in other places?
  pthread_mutex_t *seatsMutex = &SharedMemoryManager::data()->seatsMutex;
  pthread_mutex_lock(seatsMutex);

  CommissionSeat *seats = SharedMemoryManager::data()->commissionA.seats;
  for (int i = 0; i < 3; i++) {
    if (seats[i].pid == -1) {
      SharedMemoryManager::data()->commissionA.seats[i] = {
          .pid = getpid(),
          .questionsCount = 0,
          .answered = false,
      };
      pthread_mutex_unlock(seatsMutex);
      return i;
    }
  }

  pthread_mutex_unlock(seatsMutex);
  return -1;
}

/**
 * Finds a commission B seat by checking the shared memory.
 */
int CandidateProcess::findCommissionBSeat() {
  // TODO: Use mutex in other places?
  pthread_mutex_t *seatsMutex = &SharedMemoryManager::data()->seatsMutex;
  pthread_mutex_lock(seatsMutex);

  CommissionSeat *seats = SharedMemoryManager::data()->commissionB.seats;
  for (int i = 0; i < 3; i++) {
    if (seats[i].pid == -1) {
      SharedMemoryManager::data()->commissionB.seats[i] = {
          .pid = getpid(),
          .questionsCount = 0,
          .answered = false,
      };
      pthread_mutex_unlock(seatsMutex);
      return i;
    }
  }

  pthread_mutex_unlock(seatsMutex);
  return -1;
}

/**
 * Waits for the questions to be available by checking the questionsCount in the
 * shared memory.
 */
void CandidateProcess::waitForQuestions(char commission) {
  Logger::info("CandidateProcess::waitForQuestions()");
  Logger::info("Candidate process with pid " + std::to_string(getpid()) +
               " waiting for questions from commission " + commission);

  if (commission == 'A') {
    while (
        SharedMemoryManager::data()->commissionA.seats[seat].questionsCount !=
        (1 << 5) - 1) {
      int result = sleep(1);
      if (result < 0) {
        handleError("Failed to sleep in waitForQuestions()");
      }
    }
  } else {
    while (
        SharedMemoryManager::data()->commissionB.seats[seat].questionsCount !=
        (1 << 3) - 1) {
      int result = sleep(1);
      if (result < 0) {
        handleError("Failed to sleep in waitForQuestions()");
      }
    }
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

  int result = usleep(sleepTime * 1000000);
  if (result < 0) {
    handleError("Failed to sleep in prepareAnswers()");
  }

  if (commission == 'A') {
    SharedMemoryManager::data()->commissionA.seats[seat].answered = true;
  } else {
    SharedMemoryManager::data()->commissionB.seats[seat].answered = true;
  }

  Logger::info("Candidate process with pid " + std::to_string(getpid()) +
               " answered questions");
}

/**
 * Waits for the grading to be available by checking the theoreticalScore or
 * practicalScore in the shared memory.
 */
void CandidateProcess::waitForGrading(char commission) {
  Logger::info("CandidateProcess::waitForGrading()");
  if (commission == 'A') {
    while (SharedMemoryManager::data()->candidates[index].theoreticalScore <
           0) {
      int result = sleep(1);
      if (result < 0) {
        handleError("Failed to sleep in waitForGrading()");
      }
    }
  } else {
    while (SharedMemoryManager::data()->candidates[index].practicalScore < 0) {
      int result = sleep(1);
      if (result < 0) {
        handleError("Failed to sleep in waitForGrading()");
      }
    }
  }
}

/**
 * Gets a commission B seat by checking the semaphore and the shared memory.
 */
void CandidateProcess::getCommissionBSeat() {
  Logger::info("CandidateProcess::getCommissionBSeat()");

  seat = -1;
  semaphoreB = SemaphoreManager::open("commissionB");

  Logger::info("Candidate process with pid " + std::to_string(getpid()) +
               " waiting for commission B seat");

  while (seat == -1) {
    SemaphoreManager::wait(semaphoreB);
    seat = findCommissionBSeat();
    if (seat == -1) {
      SemaphoreManager::post(semaphoreB);
      int result = usleep(10000);
      if (result < 0) {
        handleError("Failed to sleep in getCommissionBSeat()");
      }
    }
  }
}

/**
 * Exits the exam if the candidate failed the exam.
 */
void CandidateProcess::maybeExitExam() {
  Logger::info("CandidateProcess::maybeExitExam()");
  if (SharedMemoryManager::data()->candidates[index].theoreticalScore < 30) {
    Logger::info("Candidate process with pid " + std::to_string(getpid()) +
                 " failed to pass the exam");
    exit(0);
  }
}

/**
 * Cleans up the candidate process.
 */
void CandidateProcess::cleanup() {
  Logger::info("CandidateProcess::cleanup()");
  SharedMemoryManager::detach();
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
