#include "commission/CommissionProcess.h"

#include "common/ipc/SharedMemoryManager.h"
#include "common/output/Logger.h"
#include "common/utils/Memory.h"
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

  semaphore =
      SemaphoreManager::open(std::string("commission") + commissionType_);

  Logger::info(std::string("Initializing comission: ") + commissionType_ +
               " with " + std::to_string(memberCount_) + " members");
}

/**
 * Sets up the signal handlers for the commission process.
 */
void CommissionProcess::setupSignalHandlers() {
  auto result = signal(SIGTERM, terminationHandler);
  if (result == SIG_ERR) {
    handleError("Failed to set up signal handler for SIGTERM");
    exit(1);
  }
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
  Logger::info("Commission " + std::string(1, commissionType_) +
               " waiting for exam start");
  while (!SharedMemoryManager::data()->examStarted) {
    int result = usleep(500000);
    if (result < 0) {
      handleError("Failed to sleep in waitForExamStart()");
    }
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
    SemaphoreManager::post(semaphore);
  }

  mainLoop();
}

/**
 * Main loop for the commission process.
 */
void CommissionProcess::mainLoop() {
  Logger::info("CommissionProcess::mainLoop() START");

  while (running) {
    pthread_mutex_lock(&SharedMemoryManager::data()->seatsMutex);

    SharedState *state = SharedMemoryManager::data();
    int totalCandidates = commissionType_ == 'A'
                              ? state->commissionACandidateCount
                              : state->commissionBCandidateCount;
    CommissionInfo *commission =
        commissionType_ == 'A' ? &state->commissionA : &state->commissionB;

    if (candidatesProcessed >= totalCandidates) {
      bool allSeatsEmpty = true;
      for (int i = 0; i < 3; i++) {
        if (commission->seats[i].pid != -1) {
          allSeatsEmpty = false;
          break;
        }
      }

      if (allSeatsEmpty) {
        Logger::info("All candidates processed (" +
                     std::to_string(candidatesProcessed) + "/" +
                     std::to_string(totalCandidates) + "), finishing...");
        running = false;

        if (commissionType_ == 'B') {
          Logger::info("Commission B finished, ending exam");
          state->examStarted = false;
        }

        pthread_mutex_unlock(&SharedMemoryManager::data()->seatsMutex);
        break;
      }
    }

    for (int i = 0; i < 3; i++) {
      if (commission->seats[i].answered && commission->seats[i].pid != -1) {
        CandidateInfo *candidate = Memory::findCandidate(commissionType_, i);

        if (candidate == nullptr) {
          Logger::warn(
              "Seat " + std::to_string(i) +
              " has answered flag but candidate not found, freeing seat");
          commission->seats[i].answered = false;
          commission->seats[i].questionsCount = 0;
          commission->seats[i].pid = -1;
          SemaphoreManager::post(semaphore);
          break;
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

          if (commissionType_ == 'A' && candidate->theoreticalScore < 30) {
            state->commissionBCandidateCount -= 1;
          }

          Logger::info("[Commission " + std::string(1, commissionType_) +
                       "] % of candidates graded: " +
                       std::to_string(candidatesProcessed /
                                      (double)totalCandidates * 100.0) +
                       "%");
        }

        commission->seats[i].answered = false;
        commission->seats[i].questionsCount = 0;
        commission->seats[i].pid = -1;

        SemaphoreManager::post(semaphore);

        break;
      }
    }

    pthread_mutex_unlock(&SharedMemoryManager::data()->seatsMutex);

    int result = usleep(1000000);
    if (result < 0) {
      handleError("Failed to sleep in mainLoop()");
    }
  }

  Logger::info("CommissionProcess::mainLoop() END");
}

/**
 * Spawns the threads for the commission members.
 */
void CommissionProcess::spawnThreads() {
  Logger::info("CommissionProcess::spawnThreads()");
  SharedState *state = SharedMemoryManager::data();
  for (int i = 0; i < memberCount_; i++) {
    threadData[i].memberId = i;
    threadData[i].commissionId = commissionType_;
    threadData[i].running = &running;
    threadData[i].mutex = &state->seatsMutex;
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

  SharedState *state = SharedMemoryManager::data();
  CommissionInfo *commission =
      (data->commissionId == 'A') ? &state->commissionA : &state->commissionB;

  int memberBit = 1 << data->memberId;

  while (*data->running) {
    int delay = Random::randomInt(2, 5);
    sleep(delay);

    if (!*data->running)
      break;

    pthread_mutex_lock(data->mutex);

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

    pthread_mutex_unlock(data->mutex);
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
  SharedMemoryManager::detach();
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
    static_cast<CommissionProcess *>(instance_)->cleanup();
    instance_ = nullptr;
  }

  Logger::info("CommissionProcess exiting with status 0 (terminated)");
  exit(0);
}
