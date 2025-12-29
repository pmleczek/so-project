#pragma once

#include "dean/DeanConfig.h"

class DeanProcess {
public:
  DeanProcess(int argc, char *argv[]);
  void initialize(int argc, char *argv[]);
  void spawnComissions();
  void spawnCandidates();
  void verifyCandidates();
  void waitForExamStart();
  void start();
  void cleanup();

private:
  int assertPlaceCount(int argc, char *argv[]);
  int assertStartTime(int argc, char *argv[]);

  int candidateCount;
  DeanConfig config;
};