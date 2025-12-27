#include "config.h"
#include "output/Logger.h"
#include "utils/random.h"

Config &Config::shared() {
  static Config instance;
  return instance;
}

const char *processTypeNames[] = {
    "main",
    "candidate",
    "examiner",
};

void Config::initializeFromArgs(int count, int time) {
  Logger::shared().log("Initializing simulation configuration");

  Logger::shared().log("Randomized configuration:");
  double ratio = Random::randomDouble(9.0, 11.0);
  Logger::shared().log("- Candidate ratio (people per place): " +
                       std::to_string(ratio));
  candidateRatio = ratio;
  // TODO: Don't divide and update ranges
  double failureRate = Random::randomDouble(1.5, 2.5) / 100;
  Logger::shared().log("- Exam failure rate: " + std::to_string(failureRate));
  examFailureRate = failureRate;
  double reattemptMultiplier = Random::randomDouble(1.5, 2.5) / 100;
  Logger::shared().log("- Reattempt rate: " + std::to_string(reattemptRate));
  reattemptRate = reattemptMultiplier;

  Logger::shared().log("Argument-based configuration:");
  candidateCount = count * candidateRatio;
  Logger::shared().log("- Candidate count: " + std::to_string(candidateCount));
  Logger::shared().log("- Start time: " + std::to_string(time));
  startTime = time;

  Logger::shared().log("Simulation configuration initialized");
}
