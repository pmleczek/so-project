#pragma once

#include "common/ipc/SemaphoreManager.h"
#include "common/process/BaseProcess.h"
#include <unistd.h>

class CandidateProcess : public BaseProcess {
public:
  CandidateProcess(int argc, char *argv[]);

  void validateArguments(int argc, char *argv[]) override;
  void initialize() override;
  void cleanup() override;
  void setupSignalHandlers() override;
  void handleError(const char *message) override;

  void waitForExamStart();
  void waitForQuestions(char commission);
  void prepareAnswers(char commission);
  void waitForGrading(char commission);
  void maybeExitExam();
  void getCommissionASeat();
  void getCommissionBSeat();

private:
  static void rejectionHandler(int signal);
  static void terminationHandler(int signal);
  int findCommissionASeat();
  int findCommissionBSeat();

  int index;
  int seat = -1;

  sem_t *semaphoreA;
  sem_t *semaphoreB;

  double timesA[5];
  double timesB[3];
};
