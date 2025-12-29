#include <iostream>

#include "common/ipc/SemaphoreManager.h"
#include "common/ipc/SharedMemoryManager.h"
#include "common/output/Logger.h"
#include "common/utils/Random.h"
#include <pthread.h>
#include <unistd.h>

std::atomic<int> candidatesProcessed = std::atomic<int>(0);

struct ThreadData {
  int memberId;
  char commissionId;
  bool *running;
  pthread_mutex_t *mutex;
};

pthread_t threadIds[3];
ThreadData threadData[3];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *threadFunction(void *arg) {
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

double getNRandomScore(int n) {
  double score = 0.0;
  for (int i = 0; i < n; i++) {
    score += Random::randomDouble(0.0, 100.0);
  }
  return score / n;
}

int main(int argc, char *argv[]) {
  Logger::info("Commission process started");
  SharedMemoryManager::attach();

  // bool allSeatsEmpty = false;
  bool running = true;

  while (!SharedMemoryManager::data()->examStarted) {
    sleep(1);
  }

  // TODO: Commission B implementation
  for (int i = 0; i < 3; ++i) {
    threadData[i] = {.memberId = i,
                     .commissionId = argv[1][0],
                     .running = &running,
                     .mutex = &mutex};

    int result =
        pthread_create(&threadIds[i], nullptr, threadFunction, &threadData[i]);
    if (result != 0) {
      Logger::error("Failed to create thread: " +
                    std::string(strerror(result)));
      return 1;
    }
  }

  // TODO: Validate args?
  sem_t *comissionSemaphore =
      SemaphoreManager::open("commission" + std::string(argv[1]));

  // TODO: Better condition for while loop?
  while (true) {
    int grading = -1;

    // DEBUG
    Logger::info("Commission A state:");
    for (int i = 0; i < 3; i++) {
      Logger::info(
          "Seat " + std::to_string(i) + ": " +
          std::to_string(
              SharedMemoryManager::data()->commissionA.seats[i].pid) +
          " " +
          std::to_string(SharedMemoryManager::data()
                             ->commissionA.seats[i]
                             .questionsCount) +
          " " +
          std::to_string(
              SharedMemoryManager::data()->commissionA.seats[i].answered));
    }
    // END DEBUG

    for (int i = 0; i < 3; i++) {
      if (grading == -1 &&
          SharedMemoryManager::data()->commissionA.seats[i].answered &&
          SharedMemoryManager::data()->commissionA.seats[i].pid != -1) {
        grading = i;
        int candidateIndex = -1;
        for (int j = 0; j < SharedMemoryManager::data()->candidateCount; j++) {
          if (SharedMemoryManager::data()->candidates[j].pid ==
              SharedMemoryManager::data()->commissionA.seats[i].pid) {
            candidateIndex = j;
            break;
          }
        }
        SharedMemoryManager::data()
            ->candidates[candidateIndex]
            .theoreticalScore = getNRandomScore(3);

        // Reset seat
        SharedMemoryManager::data()->commissionA.seats[i].answered = false;
        SharedMemoryManager::data()->commissionA.seats[i].questionsCount = 0;
        SharedMemoryManager::data()->commissionA.seats[i].pid = -1;
        SemaphoreManager::post(comissionSemaphore);
        candidatesProcessed++;
        Logger::info(
            "[Commission A] % of candidates graded: " +
            std::to_string(
                candidatesProcessed /
                (double)SharedMemoryManager::data()->commissionACandidateCount *
                100.0) +
            "%");
      }
    }

    if (candidatesProcessed < SharedMemoryManager::data()->commissionACandidateCount) {
      int freeSeats = 0;
      for (int i = 0; i < 3; i++) {
        if (SharedMemoryManager::data()->commissionA.seats[i].pid == -1) {
          freeSeats++;
        }
      }

      for (int i = 0; i < freeSeats; i++) {
        SemaphoreManager::post(comissionSemaphore);
      }
    }

    if (candidatesProcessed >=
        SharedMemoryManager::data()->commissionACandidateCount) {
      break;
    }

    // allSeatsEmpty = true;
    // for (int i = 0; i < 3; i++) {
    //   if (SharedMemoryManager::data()->commissionA.seats[i].pid != -1) {
    //     allSeatsEmpty = false;
    //     break;
    //   }
    // }

    sleep(5);
  }

  running = false;
  Logger::info("Commission " + std::string(argv[1]) + " finished work");

  for (int i = 0; i < 3; ++i) {
    void *retval;
    pthread_join(threadIds[i], &retval);
    Logger::info("Thread " + std::to_string(i) + " joined");
  }

  SharedMemoryManager::detach();

  return 0;
}
