#pragma once

#include "common/ipc/SemaphoreManager.h"

class CandidateProcess {
public:
  CandidateProcess(int argc, char *argv[]);
  void initialize(int argc, char *argv[]);
  void waitForExamStart();
  void waitForQuestions(char commission);
  void prepareAnswers(char commission);
  void waitForGrading(char commission);
  void maybeExitExam();
  void getCommissionASeat();
  void cleanup();

private:
  static void rejectionHandler(int signal);
  int findCommissionASeat();

  int index;
  int seat = -1;
  sem_t *semaphoreA;
};
