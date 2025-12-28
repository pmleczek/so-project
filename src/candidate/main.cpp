#include "common/output/Logger.h"
#include <iostream>
#include <unistd.h>

void rejectionHandler(int signal) {
  Logger::info("Candidate process with pid " + std::to_string(getpid()) +
               " exiting with status 0");
  exit(0);
}

int main(int argc, char *argv[]) {
  // signal(SIGUSR1, evacuationHandler);
  signal(SIGUSR2, rejectionHandler);

  Logger::info("Candidate process started (pid=" + std::to_string(getpid()) +
               ")");

  int n = 2;
  while (n > 0) {
    sleep(10);
    n--;
  }

  Logger::info("Candidate process with pid " + std::to_string(getpid()) +
               " exiting with status 0");
  return 0;
}
