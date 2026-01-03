#pragma once

#include <string>

class DeanConfig {
public:
  DeanConfig();
  DeanConfig(int places, int startTime);

  int placeCount;
  int startTime;
  int candidateCount;
  int failedExamCount;
  int retakeExamCount;

private:
  void printConfig();
};
