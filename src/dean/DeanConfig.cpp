#include "dean/DeanConfig.h"

#include "common/output/Logger.h"
#include "common/utils/Random.h"

DeanConfig::DeanConfig()
    : placeCount(0), startTime("00:00"), candidateCount(0), failedExamCount(0),
      retakeExamCount(0) {}

// TODO: Parse start time to seconds or smth?
DeanConfig::DeanConfig(int places, const std::string &examTime) {
  Logger::info("DeanConfig initialized with " + std::to_string(places) +
               " places");

  // Number of places available
  placeCount = places;

  // Start time of the exam
  startTime = examTime;

  // Number of candidates taking the exam
  double candidateRate = Random::randomDouble(9.5, 10.5);
  candidateCount = candidateRate * places;

  // Number of candidates who failed the exam
  double failedExamRate = Random::randomDouble(1.5, 2.5);
  failedExamCount = failedExamRate * candidateCount / 100;

  // Number of candidates who retake the exam
  double retakeExamRate = Random::randomDouble(1.5, 2.5);
  retakeExamCount = retakeExamRate * candidateCount / 100;

  printConfig();
}

void DeanConfig::printConfig() {
  Logger::info("DeanConfig - place count: " + std::to_string(placeCount) +
               " places");
  Logger::info("DeanConfig - start time: " + startTime);
  Logger::info("DeanConfig - candidate count: " +
               std::to_string(candidateCount) + " candidates");
  Logger::info("DeanConfig - failed exam count: " +
               std::to_string(failedExamCount) + " failed exams");
  Logger::info("DeanConfig - retake exam count: " +
               std::to_string(retakeExamCount) + " retake exams");
}