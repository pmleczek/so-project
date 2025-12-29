#include "commission/CommissionProcess.h"

#include "common/ipc/SharedMemoryManager.h"
#include "common/output/Logger.h"
#include "common/utils/Random.h"
#include <unistd.h>

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
  mainLoop();
}

void CommissionProcess::mainLoop() {
  Logger::info("CommissionProcess::mainLoop() START");

  while (running) {

    if (candidatesProcessed >= SharedMemoryManager::data()->candidateCount) {
      running = false;
      break;
    }

    pthread_mutex_lock(SharedMemoryManager::data()->mutex);

    for (int i = 0; i < 3; i++) {
      // TODO: Extract to function
      if (SharedMemoryManager::data()->commissionA.seats[i].answered &&
          SharedMemoryManager::data()->commissionA.seats[i].pid != -1) {
        CandidateInfo &candidate = findCandidate(i);
        if (candidate.theoreticalScore != -1) {
          continue;
        }

        candidate.theoreticalScore = getNRandomScore(3);
        candidatesProcessed++;

        SharedMemoryManager::data()->commissionA.seats[i].answered = false;
        SharedMemoryManager::data()->commissionA.seats[i].questionsCount = 0;
        SharedMemoryManager::data()->commissionA.seats[i].pid = -1;
        // SemaphoreManager::post(semaphore);

        Logger::info(
            "[Commission A] % of candidates graded: " +
            std::to_string(
                candidatesProcessed /
                (double)SharedMemoryManager::data()->commissionACandidateCount *
                100.0) +
            "%");
      }
    }

    if (candidatesProcessed <
        SharedMemoryManager::data()->commissionACandidateCount) {
      int freeSeats = 0;
      for (int i = 0; i < 3; i++) {
        if (SharedMemoryManager::data()->commissionA.seats[i].pid == -1) {
          SemaphoreManager::post(semaphore);
        }
      }
    }

    pthread_mutex_unlock(SharedMemoryManager::data()->mutex);

    usleep(1000000);
  }

  Logger::info("CommissionProcess::mainLoop() END");
}

CandidateInfo &CommissionProcess::findCandidate(int seat) {
  for (int i = 0; i < SharedMemoryManager::data()->candidateCount; i++) {
    if (SharedMemoryManager::data()->candidates[i].pid ==
        SharedMemoryManager::data()->commissionA.seats[seat].pid) {
      return SharedMemoryManager::data()->candidates[i];
    }
  }

  Logger::error("Candidate not found for seat " + std::to_string(seat));
  exit(1);
}

void CommissionProcess::spawnThreads() {
  Logger::info("CommissionProcess::spawnThreads()");
  for (int i = 0; i < memberCount_; i++) {
    threadData[i] = {.memberId = i,
                     .commissionId = commissionType_,
                     .running = &running,
                     .mutex = &mutex};
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
}
