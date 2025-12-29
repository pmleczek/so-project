#include "common/ipc/SemaphoreManager.h"
#include "common/ipc/SharedMemoryManager.h"
#include "common/output/Logger.h"
#include <iostream>
#include <unistd.h>

sem_t *commissionASemaphore;

void rejectionHandler(int signal) {
  Logger::info("Candidate process with pid " + std::to_string(getpid()) +
               " exiting with status 0");

  CommissionSeat *seats = SharedMemoryManager::data()->commissionA.seats;
  for (int i = 0; i < 3; i++) {
    if (seats[i].pid == getpid()) {
      Logger::info("Freeing up commission A seat " + std::to_string(i));
      seats[i].pid = -1;
      seats[i].answered = true;
      if (commissionASemaphore) {
        SemaphoreManager::post(commissionASemaphore);
      }
      break;
    }
  }

  exit(0);
}

int findCommissionASeat() {
  // TODO: Use mutex in other places
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

int main(int argc, char *argv[]) {
  SharedMemoryManager::attach();
  // signal(SIGUSR1, evacuationHandler);
  signal(SIGUSR2, rejectionHandler);

  while (!SharedMemoryManager::data()->examStarted) {
    sleep(1);
  }
  Logger::info("Candidate process with pid " + std::to_string(getpid()) +
               " exam started");
  Logger::info("Candidate process started (pid=" + std::to_string(getpid()) +
               ")");

  commissionASemaphore = SemaphoreManager::open("commissionA");

  Logger::info("Candidate process with pid " + std::to_string(getpid()) +
               " waiting for commission A seat");
  // SemaphoreManager::wait(commissionASemaphore);

  int seat = -1;
  while (seat == -1) {
    if (SemaphoreManager::trywait(commissionASemaphore) == 0) {
      seat = findCommissionASeat();

      if (seat == -1) {
        SemaphoreManager::post(commissionASemaphore);
        usleep(10000);
      }
    } else {
      usleep(10000);
    }
  }

  // int seat = findCommissionASeat();
  Logger::info("Candidate process with pid " + std::to_string(getpid()) +
               " got commission A seat " + std::to_string(seat));
  if (seat == -1) {
    Logger::error("Candidate process with pid " + std::to_string(getpid()) +
                  " failed to find commission A seat");
    exit(1);
  }

  while (SharedMemoryManager::data()->commissionA.seats[seat].questionsCount !=
         (1 << 3) - 1) {
    Logger::info("Candidate process with pid " + std::to_string(getpid()) +
                 " waiting for commission A seat " + std::to_string(seat) +
                 " to be asked");
    sleep(1);
  }

  // TODO: Sleep T_A_i seconds
  sleep(5);
  SharedMemoryManager::data()->commissionA.seats[seat].answered = true;
  Logger::info("Candidate process with pid " + std::to_string(getpid()) +
               " answered questions");

  Logger::info("Candidate process with pid " + std::to_string(getpid()) +
               " exiting with status 0");

  while (SharedMemoryManager::data()->candidates[0].theoreticalScore == -1) {
    sleep(1);
  }

  Logger::info(
      "Candidate process with pid " + std::to_string(getpid()) +
      " got theoretical score: " +
      std::to_string(
          SharedMemoryManager::data()->candidates[0].theoreticalScore));

  SharedMemoryManager::detach();

  return 0;
}
