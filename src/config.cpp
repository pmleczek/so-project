#include "config.h"
#include "logger.h"
#include <random>

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

  Logger::shared().log("Argument-based configuration:");
  Logger::shared().log("- Candidate count: " + std::to_string(count));
  candidateCount = count;
  Logger::shared().log("- Start time: " + std::to_string(time));
  startTime = time;

  Logger::shared().log("Randomized configuration:");
  double ratio = randomDouble(9.0, 11.0);
  Logger::shared().log("- Candidate ratio (people per place): " +
                       std::to_string(ratio));
  candidateRatio = ratio;
  double failureRate = randomDouble(1.5, 2.5);
  Logger::shared().log("- Exam failure rate (%): " +
                       std::to_string(failureRate));
  examFailureRate = failureRate;
  double reattemptRate = randomDouble(1.5, 2.5);
  Logger::shared().log("- Reattempt rate (%): " +
                       std::to_string(reattemptRate));
  reattemptRate = reattemptRate;

  Logger::shared().log("Simulation configuration initialized");
}

double Config::randomDouble(double min, double max) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dis(min, max);
  return dis(gen);
}
