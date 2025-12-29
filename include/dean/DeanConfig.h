#pragma once

#include <string>

class DeanConfig {
public:
  DeanConfig();
  DeanConfig(int places, const std::string &startTime);

  int placeCount;
  std::string startTime;
  int candidateCount;
  int failedExamCount;
  int retakeExamCount;

private:
  void printConfig();
};
