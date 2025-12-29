#include "common/ipc/SemaphoreManager.h"
#include "common/ipc/SharedMemoryManager.h"
#include "common/output/Logger.h"
#include <ctime>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

std::vector<int> spawnCandidates(int count) {
  std::vector<int> pids;

  for (int i = 0; i < count; i++) {
    int pid = fork();
    if (pid == 0) {
      execlp("./candidate", "./candidate", std::to_string(i).c_str(), NULL);
    } else {
      // Inicjalizuj kandydata - wszystkie pola mają wartości domyślne z
      // struktury
      SharedMemoryManager::data()->candidates[i].pid = pid;
      SharedMemoryManager::data()->candidates[i].theoreticalScore = -1.0;
      SharedMemoryManager::data()->candidates[i].practicalScore = -1.0;
      SharedMemoryManager::data()->candidates[i].finalScore = -1.0;
      SharedMemoryManager::data()->candidates[i].status = Pending;
      pids.push_back(pid);
    }
  }

  return pids;
}

int timeToSeconds(const std::string &timeStr) {
  int h, m;
  char colon;
  std::istringstream iss(timeStr);
  iss >> h >> colon >> m;
  return h * 3600 + m * 60;
}

int currentTimeSeconds() {
  std::time_t now = std::time(nullptr);
  std::tm *local = std::localtime(&now);
  return local->tm_hour * 3600 + local->tm_min * 60 + local->tm_sec;
}

void waitForStart(const std::string &startTime) {
  Logger::info("Waiting for start time: " + startTime);
  int start = timeToSeconds(startTime);
  int current = currentTimeSeconds();
  int sleepTime = start - current;

  if (sleepTime < 0) {
    // TODO: Add better sleep logic or handle invalid start time
    sleepTime = 15;
  }

  Logger::info("Sleeping for " + std::to_string(sleepTime) + " seconds");
  sleep(sleepTime);
}

void rejectCandidates(const std::vector<int> &candidatePids) {
  // TODO: Read from shared memory
  int rejected = 0;
  for (int i = 0; i < candidatePids.size(); i++) {
    if (i % 15 == 0) {
      Logger::info("Rejecting candidate " + std::to_string(i) + " (pid=" +
                   std::to_string(candidatePids[i]) + ") due to exam failure");
      SharedMemoryManager::data()->candidates[i].status = NotEligible;
      kill(candidatePids[i], SIGUSR2);
      rejected++;
    } else {
      SharedMemoryManager::data()->candidates[i].status = PendingCommissionA;
    }
  }

  SharedMemoryManager::data()->commissionACandidateCount =
      candidatePids.size() - rejected;
}

void spawnComissions() {
  // Comission A
  pid_t pidA = fork();
  if (pidA == 0) {
    execlp("./commission", "./commission", "A", NULL);
  }

  // Comission B
  // TODO: Uncomment when commission division is implemented
  // pid_t pidB = fork();
  // if (pidB == 0) {
  //   execlp("./commission", "./commission", "B", NULL);
  // }
}

void initialize() {
  Logger::setupLogFile();
  Logger::info("Dean process started (pid=" + std::to_string(getpid()) + ")");

  SharedMemoryManager::initialize(100);
  // Semafor startuje z 0 - komisja zwolni 3 miejsca po rozpoczęciu egzaminu
  SemaphoreManager::create("commissionA", 0);
  SemaphoreManager::create("commissionB", 0);

  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&SharedMemoryManager::data()->seatsMutex, &attr);
  pthread_mutexattr_destroy(&attr);

  for (int i = 0; i < 3; i++) {
    SharedMemoryManager::data()->commissionA.seats[i].pid = -1;
    SharedMemoryManager::data()->commissionA.seats[i].questionsCount = 0;
    SharedMemoryManager::data()->commissionA.seats[i].answered = false;

    // TODO: Uncomment when commission division is implemented
    // SharedMemoryManager::data()->commissionB.seats[i].pid = -1;
    // SharedMemoryManager::data()->commissionB.seats[i].questionsCount = 0;
    // SharedMemoryManager::data()->commissionB.seats[i].answered = false;
  }
}

void cleanup() {
  SharedMemoryManager::destroy();
  SemaphoreManager::unlink("/commissionA");
  SemaphoreManager::unlink("/commissionB");

  Logger::info("Dean process finished");
}

int main(int argc, char *argv[]) {
  initialize();

  int candidateCount = 100;
  std::string startTime = "00:02";

  spawnComissions();
  std::vector<int> candidatePids = spawnCandidates(candidateCount);

  // Ustaw liczbę kandydatów w shared memory
  SharedMemoryManager::data()->candidateCount = candidateCount;

  waitForStart(startTime);
  // TODO: Or wait some time for processes to start
  rejectCandidates(candidatePids);

  SharedMemoryManager::data()->examStarted = true;

  while (wait(NULL) > 0) { /* no-op */
    ;
  }

  cleanup();

  return 0;
}
