#include "common/ipc/SemaphoreManager.h"
#include "common/ipc/SharedMemoryManager.h"
#include "common/output/Logger.h"
#include "candidate/CandidateProcess.h"
#include <iostream>
#include <unistd.h>

int main(int argc, char *argv[]) {
  CandidateProcess candidateProcess(argc, argv);

  candidateProcess.waitForExamStart();
  candidateProcess.getCommissionASeat();

  candidateProcess.waitForQuestions('A');
  candidateProcess.prepareAnswers('A');
  candidateProcess.waitForGrading('A');

  candidateProcess.maybeExitExam();

  // TODO: Implement commission B

  candidateProcess.cleanup();

  return 0;
}
