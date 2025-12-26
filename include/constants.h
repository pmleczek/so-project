#pragma once

#include <string>

struct Constants {
  static constexpr const char *LOG_FILE = "simulation.log";
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
