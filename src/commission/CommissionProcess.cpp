#include "commission/CommissionProcess.h"

#include "common/ipc/MutexWrapper.h"
#include "common/ipc/SharedMemoryManager.h"
#include "common/output/Logger.h"
#include "common/utils/Memory.h"
#include "common/utils/Misc.h"
#include "common/utils/Random.h"
#include <signal.h>
#include <unistd.h>

/**
 * Constructor for the commission process.
 *
 * @param argc The number of arguments passed to the program.
 * @param argv The arguments passed to the program.
 */
CommissionProcess::CommissionProcess(int argc, char *argv[])
    : BaseProcess(argc, argv, false), memberCount_(argc) {
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
void CommissionProcess::validateArguments(int argc, char *argv[]) {
  if (argc != 2) {
    throw std::invalid_argument("Invalid number of arguments. Usage: "
                                "./commission <type>");
  }

  commissionType_ = argv[1][0];
  if (commissionType_ != 'A' && commissionType_ != 'B') {
    throw std::invalid_argument(
        "Invalid commission type. Usage: ./commission <type: A or B>");
  }
  Logger::setProcessPrefix(
      "Commission (type=" + std::string(1, commissionType_) + ")");

  if (commissionType_ == 'A') {
    memberCount_ = 5;
  } else if (commissionType_ == 'B') {
    memberCount_ = 3;
  }
}

/**
 * Initializes the commission process.
 */
void CommissionProcess::initialize() {
  SharedMemoryManager::attach();

  try {
    semaphore =
        SemaphoreManager::open(std::string("commission") + commissionType_);
  } catch (const std::exception &e) {
    std::string errorMessage =
        "Failed to open semaphore: " + std::string(e.what());
    handleError(errorMessage.c_str());
    exit(1);
  }

  Logger::info(std::string("Initializing comission: ") + commissionType_ +
               " with " + std::to_string(memberCount_) + " members");
}

/**
 * Sets up the signal handlers for the commission process.
 */
void CommissionProcess::setupSignalHandlers() {
  registerSignal(SIGTERM, CommissionProcess::terminationHandler);
}

/**
 * Handles an error by sending a SIGTERM to the dean process and exiting the
 * process with status 1.
 *
 * @param message The message to display.
 */
void CommissionProcess::handleError(const char *message) {
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
void CommissionProcess::waitForExamStart() {
  pthread_mutex_t *examStateMutex =
      &SharedMemoryManager::data()->examStateMutex;

  try {
    Logger::info("Commission " + std::string(1, commissionType_) +
                 " waiting for exam start");
    while (true) {
      MutexWrapper::lock(examStateMutex);
      if (SharedMemoryManager::data()->examStarted) {
        MutexWrapper::unlock(examStateMutex);
        break;
      }
      MutexWrapper::unlock(examStateMutex);

      Misc::safeUSleep(500000);
    }
  } catch (const std::exception &e) {
    std::string errorMessage =
        "Failed to wait for exam start: " + std::string(e.what());
    handleError(errorMessage.c_str());
  }
}

/**
 * Starts the commission process.
 */
void CommissionProcess::start() {
  Logger::info("CommissionProcess::start()");
  spawnThreads();

  Logger::info("Commission " + std::string(1, commissionType_) +
               " releasing 3 seats after exam start");
  for (int i = 0; i < 3; i++) {
    try {
      SemaphoreManager::post(semaphore);
    } catch (const std::exception &e) {
      std::string errorMessage =
          "Failed to post to semaphore: " + std::string(e.what());
      handleError(errorMessage.c_str());
    }
  }

  mainLoop();
}

/**
 * Gets the commission info for the given commission type.
 *
 * @return The commission info.
 */
CommissionInfo *CommissionProcess::commission() {
  return commissionType_ == 'A' ? &SharedMemoryManager::data()->commissionA
                                : &SharedMemoryManager::data()->commissionB;
}

/**
 * Gets the total number of candidates for the given commission type.
 *
 * @return The total number of candidates.
 */
int CommissionProcess::totalCandidates() {
  return commissionType_ == 'A'
             ? SharedMemoryManager::data()->commissionACandidateCount
             : SharedMemoryManager::data()->commissionBCandidateCount;
}

/**
 * Main loop for the commission process.
 */
void CommissionProcess::mainLoop() {
  pthread_mutex_t *commissionMutex = nullptr;
  if (commissionType_ == 'A') {
    commissionMutex = &SharedMemoryManager::data()->commissionAMutex;
  } else {
    commissionMutex = &SharedMemoryManager::data()->commissionBMutex;
  }

  try {
    while (running) {
      /* Check if all candidates have been graded and commission is empty */
      maybeFinish();

      /* Grade one candidate if they have answered the questions */
      for (int i = 0; i < 3; i++) {
        if (maybeGradeCandidate(i)) {
          MutexWrapper::lock(commissionMutex);
          Memory::resetSeat(commissionType_, i);
          MutexWrapper::unlock(commissionMutex);
          SemaphoreManager::post(semaphore);
          break;
        }
      }

      /* Wait for next commission iteration*/
      Misc::safeUSleep(1000000);
    }
  } catch (const std::exception &e) {
    std::string errorMessage =
        "Failed in commission mainLoop: " + std::string(e.what());
    handleError(errorMessage.c_str());
  }
}

/**
 * Spawns the threads for the commission members.
 */
void CommissionProcess::spawnThreads() {
  Logger::info("CommissionProcess::spawnThreads()");

  pthread_mutex_t *commissionMutex = nullptr;
  if (commissionType_ == 'A') {
    commissionMutex = &SharedMemoryManager::data()->commissionAMutex;
  } else {
    commissionMutex = &SharedMemoryManager::data()->commissionBMutex;
  }

  for (int i = 0; i < memberCount_; i++) {
    threadData[i].memberId = i;
    threadData[i].commissionId = commissionType_;
    threadData[i].running = &running;
    threadData[i].mutex = commissionMutex;
    int result =
        pthread_create(&threadIds[i], nullptr, threadFunction, &threadData[i]);
    if (result != 0) {
      Logger::error("Failed to create thread: " + std::to_string(result));
      exit(1);
    }
  }
}

/**
 * Waits for the threads to finish execution.
 */
void CommissionProcess::waitThreads() {
  Logger::info("CommissionProcess::waitThreads()");
  for (int i = 0; i < memberCount_; i++) {
    pthread_join(threadIds[i], nullptr);
  }
}

/**
 * Thread function for the commission members.
 */
void *CommissionProcess::threadFunction(void *arg) {
  ThreadData *data = static_cast<ThreadData *>(arg);

  Logger::info("Commission " + std::string(1, data->commissionId) + " member " +
               std::to_string(data->memberId) + " started");

  int memberBit = 1 << data->memberId;

  while (*data->running) {
    int delay = Random::randomInt(2, 5);
    try {
      Misc::safeSleep(delay);
    } catch (const std::exception &e) {
      std::string errorMessage =
          "Failed to sleep in threadFunction: " + std::string(e.what());
      static_cast<CommissionProcess *>(instance_)->handleError(
          errorMessage.c_str());
    }

    if (!*data->running)
      break;

    MutexWrapper::lock(data->mutex);
    SharedState *state = SharedMemoryManager::data();
    CommissionInfo *commission =
        (data->commissionId == 'A') ? &state->commissionA : &state->commissionB;

    for (int seat = 0; seat < 3; ++seat) {
      if (commission->seats[seat].pid != -1 &&
          !(commission->seats[seat].questionsCount & memberBit)) {
        commission->seats[seat].questionsCount |= memberBit;

        int pid = commission->seats[seat].pid;

        Logger::info("Member " + std::to_string(data->memberId) +
                     " generated question for seat " + std::to_string(seat) +
                     " taken by pid " + std::to_string(pid));
      }
    }

    MutexWrapper::unlock(data->mutex);
  }

  Logger::info("Member " + std::to_string(data->memberId) + " finished work");
  pthread_exit(nullptr);
  return nullptr;
}

/**
 * Cleans up the commission process.
 */
void CommissionProcess::cleanup() {
  Logger::info("CommissionProcess::cleanup()");
  waitThreads();

  try {
    SharedMemoryManager::detach();
    SemaphoreManager::close(semaphore);
  } catch (const std::exception &e) {
    std::string errorMessage =
        "Failed to cleanup the commission process: " + std::string(e.what());
    perror(errorMessage.c_str());
  }

  Logger::info("CommissionProcess exiting");
}

/**
 * Handles the termination signal for the commission process.
 */
void CommissionProcess::terminationHandler(int signal) {
  Logger::info("CommissionProcess::terminationHandler()");
  Logger::info("Termination signal: SIG " + std::to_string(signal) +
               " received");

  if (instance_) {
    auto commissionProcess = static_cast<CommissionProcess *>(instance_);
    commissionProcess->running = false;

    for (int i = 0; i < memberCount_; i++) {
      int result = pthread_cancel(commissionProcess->threadIds[i]);
      if (result != 0) {
        std::string errorMessage =
            "Failed to cancel thread: " + std::to_string(result);
        perror(errorMessage.c_str());
      }
    }

    commissionProcess->cleanup();
    instance_ = nullptr;
  }

  Logger::info("CommissionProcess exiting with status 0 (terminated)");
  exit(0);
}

/**
 * Grades a candidate if they have answered the questions.
 *
 * @param seat The seat of the candidate.
 * @return True if the candidate has been graded, false otherwise.
 */
bool CommissionProcess::maybeGradeCandidate(int seat) {
  pthread_mutex_t *commissionMutex = nullptr;
  if (commissionType_ == 'A') {
    commissionMutex = &SharedMemoryManager::data()->commissionAMutex;
  } else {
    commissionMutex = &SharedMemoryManager::data()->commissionBMutex;
  }

  pthread_mutex_t *candidatesMutex =
      &SharedMemoryManager::data()->candidateMutex;

  pthread_mutex_t *examStateMutex =
      &SharedMemoryManager::data()->examStateMutex;

  try {
    MutexWrapper::lock(commissionMutex);
    CommissionInfo *commissionInfo = commission();

    if (!commissionInfo->seats[seat].answered ||
        commissionInfo->seats[seat].pid == -1) {
      MutexWrapper::unlock(commissionMutex);
      return false;
    }

    MutexWrapper::lock(candidatesMutex);
    CandidateInfo *candidate = Memory::findCandidate(commissionType_, seat);
    if (candidate == nullptr) {
      Logger::warn("Seat " + std::to_string(seat) +
                   " has answered flag but candidate not found, freeing seat");

      MutexWrapper::unlock(candidatesMutex);

      Memory::resetSeat(commissionType_, seat);
      MutexWrapper::unlock(commissionMutex);

      SemaphoreManager::post(semaphore);

      return false;
    }

    double score = commissionType_ == 'A' ? candidate->theoreticalScore
                                          : candidate->practicalScore;
    if (score < 0) {
      if (commissionType_ == 'A') {
        candidate->theoreticalScore = Random::sampleMean(5, 0.0, 100.0);
      } else {
        candidate->practicalScore = Random::sampleMean(3, 0.0, 100.0);
      }

      candidatesProcessed++;

      MutexWrapper::lock(examStateMutex);
      if (commissionType_ == 'A' && candidate->theoreticalScore < 30) {
        SharedMemoryManager::data()->commissionBCandidateCount -= 1;
      }

      double percentage =
          candidatesProcessed / (double)totalCandidates() * 100.0;

      MutexWrapper::unlock(examStateMutex);

      Logger::info("% of candidates graded: " + std::to_string(percentage) +
                   "%");

      MutexWrapper::unlock(candidatesMutex);
      MutexWrapper::unlock(commissionMutex);

      return true;
    }

    MutexWrapper::unlock(candidatesMutex);
    MutexWrapper::unlock(commissionMutex);

    return false;
  } catch (const std::exception &e) {
    std::string errorMessage =
        "Failed to grade candidate: " + std::string(e.what());
    handleError(errorMessage.c_str());
  }
}

/**
 * Finishes the commission process if all candidates have been graded.
 */
void CommissionProcess::maybeFinish() {
  pthread_mutex_t *examStateMutex =
      &SharedMemoryManager::data()->examStateMutex;

  pthread_mutex_t *commissionMutex = nullptr;
  if (commissionType_ == 'A') {
    commissionMutex = &SharedMemoryManager::data()->commissionAMutex;
  } else {
    commissionMutex = &SharedMemoryManager::data()->commissionBMutex;
  }

  try {
    MutexWrapper::lock(examStateMutex);

    if (candidatesProcessed >= totalCandidates()) {
      bool allSeatsEmpty = true;

      MutexWrapper::lock(commissionMutex);
      CommissionInfo *commissionInfo = commission();

      for (int i = 0; i < 3; i++) {
        if (commissionInfo->seats[i].pid != -1) {
          Logger::info(std::string("Seat ") + std::to_string(i) +
                       " is not empty");
          allSeatsEmpty = false;
          break;
        }
      }

      MutexWrapper::unlock(commissionMutex);

      if (allSeatsEmpty) {
        Logger::info("All candidates processed (" +
                     std::to_string(candidatesProcessed) + "/" +
                     std::to_string(totalCandidates()) + "), finishing...");
        running = false;

        if (commissionType_ == 'B') {
          Logger::info("Commission B finished, ending exam");
          SharedMemoryManager::data()->examStarted = false;
        }
      }

      MutexWrapper::unlock(examStateMutex);
      return;
    }

    MutexWrapper::unlock(examStateMutex);
  } catch (const std::exception &e) {
    std::string errorMessage =
        "Failed to maybe finish: " + std::string(e.what());
    handleError(errorMessage.c_str());
  }
}
