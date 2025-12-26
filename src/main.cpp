#include "config.h"
#include "logger.h"
#include "validators.h"
#include <iostream>
#include <unistd.h>

int main(int argc, char *argv[]) {
  // Main process PID
  pid_t mainPid = getpid();
  Logger::shared().log("Starting main process");
  Logger::shared().log("PID: " + std::to_string(mainPid));
  Config::shared().registerProcess(mainPid, MAIN);

  // Validate arguments and initialize config
  Logger::shared().log("Validating arguments and initializing config");
  // Validate arguments
  ArgValidator::validateArgs(argc, argv);
  // Generate simulation config based on
  // args and randomized values
  Config::shared().initializeFromArgs(std::stoi(argv[1]), std::stoi(argv[2]));

  // Wait until exam begins
  int startTime = Config::shared().startTime;
  Logger::shared().log("Waiting for the exam to begin for: " +
                       std::to_string(startTime) + " seconds");
  sleep(startTime);

  // Start exam simulation
  Logger::shared().log("Starting exam simulation");

  return 0;
}
