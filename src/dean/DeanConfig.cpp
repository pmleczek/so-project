#include "dean/DeanConfig.h"

#include "common/output/Logger.h"
#include "common/utils/Random.h"

DeanConfig::DeanConfig()
    : placeCount(0), startTime(0), candidateCount(0), failedExamCount(0),
      retakeExamCount(0), timesA{0.0, 0.0, 0.0, 0.0, 0.0},
      timesB{0.0, 0.0, 0.0} {}

DeanConfig::DeanConfig(int places, int startTime) {
  Logger::info("DeanConfig initialized with: " + std::to_string(places) +
               " places and start time of: " + std::to_string(startTime));

  // Number of places available
  placeCount = places;

  // Start time of the exam
  this->startTime = startTime;

  // Number of candidates taking the exam
  double candidateRate = Random::randomDouble(9.5, 10.5);
  candidateCount = candidateRate * places;

  // Number of candidates who failed the exam
  double failedExamRate = Random::randomDouble(1.5, 2.5);
  failedExamCount = failedExamRate * candidateCount / 100;

  // Number of candidates who retake the exam
  double retakeExamRate = Random::randomDouble(1.5, 2.5);
  retakeExamCount = retakeExamRate * candidateCount / 100;

  // Times for answers in commission A
  timesA[0] = Random::randomDouble(0.25, 1.0);
  timesA[1] = Random::randomDouble(0.25, 1.0);
  timesA[2] = Random::randomDouble(0.25, 1.0);
  timesA[3] = Random::randomDouble(0.25, 1.0);
  timesA[4] = Random::randomDouble(0.25, 1.0);

  // Times for answers in commission B
  timesB[0] = Random::randomDouble(0.25, 1.0);
  timesB[1] = Random::randomDouble(0.25, 1.0);
  timesB[2] = Random::randomDouble(0.25, 1.0);

  printConfig();
}

void DeanConfig::printConfig() {
  Logger::info("DeanConfig - place count: " + std::to_string(placeCount) +
               " places");
  Logger::info("DeanConfig - start time: " + std::to_string(startTime));
  Logger::info("DeanConfig - candidate count: " +
               std::to_string(candidateCount) + " candidates");
  Logger::info("DeanConfig - failed exam count: " +
               std::to_string(failedExamCount) + " failed exams");
  Logger::info("DeanConfig - retake exam count: " +
               std::to_string(retakeExamCount) + " retake exams");
  Logger::info("DeanConfig - times for answers in commission A: " +
               std::to_string(timesA[0]) + ", " + std::to_string(timesA[1]) +
               ", " + std::to_string(timesA[2]) + ", " +
               std::to_string(timesA[3]) + ", " + std::to_string(timesA[4]));
  Logger::info("DeanConfig - times for answers in commission B: " +
               std::to_string(timesB[0]) + ", " + std::to_string(timesB[1]) +
               ", " + std::to_string(timesB[2]));
}