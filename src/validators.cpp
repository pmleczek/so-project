#include "validators.h"

#include "constants.h"
#include "logger.h"
#include <iostream>

void ArgValidator::validateArgs(int argc, char *argv[]) {
  Logger::shared().log("Validating passed argument count: " +
                       std::to_string(argc));
  if (!validateArgCount(argc)) {
    std::cerr << "Invalid argument count" << std::endl;
    exit(1);
    // TODO: Add proper error handling
  }

  int candidateCount = std::stoi(argv[1]);
  Logger::shared().log("Validating passed candidate count: " +
                       std::to_string(candidateCount));
  if (!validateCandidateCount(candidateCount)) {
    std::cerr << "Invalid candidate count" << std::endl;
    exit(1);
    // TODO: Add proper error handling
  }

  int startTime = std::stoi(argv[2]);
  Logger::shared().log("Validating passed start time: " +
                       std::to_string(startTime));
  if (!validateStartTime(startTime)) {
    std::cerr << "Invalid start time" << std::endl;
    exit(1);
    // TODO: Add proper error handling
  }

  Logger::shared().log("All passed arguments validated successfully");
}

bool ArgValidator::validateArgCount(int argc) {
  return argc - 1 == ArgConstants::ARG_COUNT;
}

bool ArgValidator::validateCandidateCount(int candidateCount) {
  return candidateCount >= ArgConstants::CANDIDATE_COUNT_MIN;
}

bool ArgValidator::validateStartTime(int startTime) {
  return startTime >= ArgConstants::TIME_MIN;
}
