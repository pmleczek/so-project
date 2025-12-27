#pragma once

#include <string>

struct GeneralConstants {
  static constexpr const char *LOG_FILENAME = "simulation.log";
  static constexpr const char *RESULTS_FILENAME = "results.csv";
  static constexpr const char *OUTPUT_DIRECTORY = "output";
};

// Constants for argument validation
struct ArgConstants {
  // General arg count
  static const int ARG_COUNT = 2;
  // Candidate count constraints
  static const int CANDIDATE_COUNT_MIN = 1;
  // Time constraints
  static const int TIME_MIN = 0;
};

class Constants {
public:
  static std::string logFilePath();
  static std::string resultsFilePath();
  static std::string outputDirectory();
};
