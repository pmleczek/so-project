#include "commission/CommissionProcess.h"

#include "common/ipc/SharedMemoryManager.h"
#include "common/output/Logger.h"
#include "common/utils/Random.h"
#include <unistd.h>

CommissionProcess *CommissionProcess::instance_ = nullptr;

// TODO: Extract
double getNRandomScore(int n) {
  double score = 0.0;
  for (int i = 0; i < n; i++) {
    score += Random::randomDouble(0.0, 100.0);
  }
  return score / n;
}

CommissionProcess::CommissionProcess(int argc, char *argv[])
    : memberCount_(argc) {
  Logger::info("CommissionProcess::CommissionProcess()");
  instance_ = this;
  initialize(argc, argv);
}

void CommissionProcess::initialize(int argc, char *argv[]) {
  Logger::info("CommissionProcess::initialize()");
  SharedMemoryManager::attach();

  // TODO: Validate args
  if (argc != 2) {
    Logger::error("Invalid number of arguments: " + std::to_string(argc));
    exit(1);
  }

  commissionType_ = argv[1][0];
  semaphore =
      SemaphoreManager::open(std::string("commission") + commissionType_);

  if (commissionType_ == 'A') {
    memberCount_ = 5;
  } else if (commissionType_ == 'B') {
    memberCount_ = 3;
  }

  Logger::info(std::string("Initializing comission: ") + commissionType_ +
               " with " + std::to_string(memberCount_) + " members");
}

void CommissionProcess::waitForExamStart() {
  Logger::info("CommissionProcess::waitForExamStart()");
  while (!SharedMemoryManager::data()->examStarted) {
    usleep(500000);
  }
}

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

void CommissionProcess::mainLoop() {
  Logger::info("CommissionProcess::mainLoop() START");

  while (running) {
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
          SharedMemoryManager::data()->examStarted = false;
        }

        break;
      }
    }

    pthread_mutex_lock(&state->seatsMutex);

    for (int i = 0; i < 3; i++) {
      // TODO: Extract to function
      if (commission->seats[i].answered && commission->seats[i].pid != -1) {
        CandidateInfo *candidate = findCandidate(i);

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
            candidate->theoreticalScore = getNRandomScore(5);
          } else {
            candidate->practicalScore = getNRandomScore(3);
          }
          candidatesProcessed++;

          if (commissionType_ == 'A' && candidate->theoreticalScore < 30) {
            SharedMemoryManager::data()->commissionBCandidateCount -= 1;
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

    pthread_mutex_unlock(&state->seatsMutex);

    usleep(1000000);
  }

  Logger::info("CommissionProcess::mainLoop() END");
}

CandidateInfo *CommissionProcess::findCandidate(int seat) {
  SharedState *state = SharedMemoryManager::data();
  CommissionInfo *commission =
      commissionType_ == 'A' ? &state->commissionA : &state->commissionB;
  int pid = commission->seats[seat].pid;

  for (int i = 0; i < state->candidateCount; i++) {
    if (state->candidates[i].pid == pid) {
      return &state->candidates[i];
    }
  }

  Logger::warn("Candidate not found for seat " + std::to_string(seat) +
               " with pid " + std::to_string(pid) +
               " - candidate may have already exited");
  return nullptr;
}

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

void CommissionProcess::waitThreads() {
  Logger::info("CommissionProcess::waitThreads()");
  for (int i = 0; i < memberCount_; i++) {
    pthread_join(threadIds[i], nullptr);
  }
}

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

void CommissionProcess::cleanup() {
  Logger::info("CommissionProcess::cleanup()");
  waitThreads();
  SharedMemoryManager::detach();
  Logger::info("CommissionProcess exiting");
}

void CommissionProcess::terminationHandler(int signal) {
  Logger::info("CommissionProcess::terminationHandler()");
  Logger::info("Termination signal: SIG " + std::to_string(signal) + " received");

  if (instance_) {
    instance_->cleanup();
    instance_ = nullptr;
  }

  Logger::info("CommissionProcess exiting with status 0 (terminated)");
  exit(0);
}
