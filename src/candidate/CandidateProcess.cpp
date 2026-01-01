#include "candidate/CandidateProcess.h"
#include "common/ipc/SharedMemoryManager.h"
#include "common/output/Logger.h"

#include <unistd.h>

CandidateProcess::CandidateProcess(int argc, char *argv[]) {
  Logger::info("Candidate process started (pid=" + std::to_string(getpid()) +
               ")");

  // TODO: Implement
  // signal(SIGUSR1, evacuationHandler);
  signal(SIGUSR2, rejectionHandler);

  initialize(argc, argv);
}

void CandidateProcess::initialize(int argc, char *argv[]) {
  Logger::info("CandidateProcess::initialize()");
  SharedMemoryManager::attach();

  // TODO: Validate args
  index = std::stoi(argv[1]);
}

void CandidateProcess::waitForExamStart() {
  Logger::info("CandidateProcess::waitForExamStart()");
  while (!SharedMemoryManager::data()->examStarted) {
    sleep(1);
  }
}

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
  SemaphoreManager::wait(semaphoreA);

  while (seat == -1) {
    if (SemaphoreManager::trywait(semaphoreA) == 0) {
      seat = findCommissionASeat();
      if (seat == -1) {
        SemaphoreManager::post(semaphoreA);
        usleep(10000);
      }
    } else {
      usleep(10000);
    }
  }
}

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

void CandidateProcess::waitForQuestions(char commission) {
  Logger::info("CandidateProcess::waitForQuestions()");
  Logger::info("Candidate process with pid " + std::to_string(getpid()) +
               " waiting for questions from commission " + commission);

  if (commission == 'A') {
    while (
        SharedMemoryManager::data()->commissionA.seats[seat].questionsCount !=
        (1 << 5) - 1) {
      sleep(1);
    }
  } else {
    while (
        SharedMemoryManager::data()->commissionB.seats[seat].questionsCount !=
        (1 << 3) - 1) {
      sleep(1);
    }
  }
}

void CandidateProcess::prepareAnswers(char commission) {
  // TODO: Proper implementation
  // TODO: Sleep T_A_i seconds
  sleep(5);

  if (commission == 'A') {
    SharedMemoryManager::data()->commissionA.seats[seat].answered = true;
  } else {
    SharedMemoryManager::data()->commissionB.seats[seat].answered = true;
  }

  Logger::info("Candidate process with pid " + std::to_string(getpid()) +
               " answered questions");
}

void CandidateProcess::waitForGrading(char commission) {
  Logger::info("CandidateProcess::waitForGrading()");
  if (commission == 'A') {
    while (SharedMemoryManager::data()->candidates[index].theoreticalScore <
           0) {
      sleep(1);
    }
  } else {
    while (SharedMemoryManager::data()->candidates[index].practicalScore < 0) {
      sleep(1);
    }
  }
}

void CandidateProcess::getCommissionBSeat() {
  Logger::info("CandidateProcess::getCommissionBSeat()");

  seat = -1;
  semaphoreB = SemaphoreManager::open("commissionB");

  Logger::info("Candidate process with pid " + std::to_string(getpid()) +
               " waiting for commission B seat");
  SemaphoreManager::wait(semaphoreB);

  while (seat == -1) {
    if (SemaphoreManager::trywait(semaphoreB) == 0) {
      seat = findCommissionBSeat();
      if (seat == -1) {
        SemaphoreManager::post(semaphoreB);
        usleep(10000);
      }
    } else {
      usleep(10000);
    }
  }
}

void CandidateProcess::maybeExitExam() {
  Logger::info("CandidateProcess::maybeExitExam()");
  if (SharedMemoryManager::data()->candidates[index].theoreticalScore < 30) {
    Logger::info("Candidate process with pid " + std::to_string(getpid()) +
                 " failed to pass the exam");
    exit(0);
  }
}

void CandidateProcess::cleanup() {
  Logger::info("CandidateProcess::cleanup()");
  SharedMemoryManager::detach();
  Logger::info("Candidate process with pid " + std::to_string(getpid()) +
               " exiting with status 0");
}

void CandidateProcess::rejectionHandler(int signal) {
  Logger::info("CandidateProcess::rejectionHandler()");
  Logger::info("Candidate process with pid " + std::to_string(getpid()) +
               " exiting with status 0");
  exit(0);
}
