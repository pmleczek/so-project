#include "common/output/Logger.h"
#include <ctime>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <unistd.h>
#include <vector>

std::vector<int> spawnCandidates(int count) {
  std::vector<int> pids;

  for (int i = 0; i < count; i++) {
    int pid = fork();
    if (pid == 0) {
      execlp("./candidate", "./candidate", NULL);
    } else {
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
  for (int i = 0; i < candidatePids.size(); i++) {
    if (i % 15 == 0) {
      Logger::info("Rejecting candidate " + std::to_string(i) + " (pid=" +
                   std::to_string(candidatePids[i]) + ") due to exam failure");
      kill(candidatePids[i], SIGUSR2);
    }
  }
}

void makeQueueForCommissionA() {
  Logger::info("Making queue for commission A");
}

int main(int argc, char *argv[]) {
  Logger::setupLogFile();
  Logger::info("Dean process started (pid=" + std::to_string(getpid()) + ")");

  int candidateCount = 100;
  std::string startTime = "14:06";

  std::vector<int> candidatePids = spawnCandidates(candidateCount);
  waitForStart(startTime);
  // TODO: Or wait some time for processes to start
  rejectCandidates(candidatePids);

  makeQueueForCommissionA();

  return 0;
}
