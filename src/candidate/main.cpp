#include "common/ipc/SemaphoreManager.h"
#include "common/ipc/SharedMemoryManager.h"
#include "common/output/Logger.h"
#include "candidate/CandidateProcess.h"
#include <iostream>
#include <unistd.h>

int main(int argc, char *argv[]) {
  CandidateProcess candidateProcess(argc, argv);

  candidateProcess.waitForExamStart();
  
  if (!candidateProcess.isRetaking()) {
    candidateProcess.getCommissionSeat('A');
    candidateProcess.waitForQuestions('A');
    candidateProcess.prepareAnswers('A');
    candidateProcess.waitForGrading('A');
    candidateProcess.maybeExitExam();
  }

  candidateProcess.getCommissionSeat('B');
  candidateProcess.waitForQuestions('B');
  candidateProcess.prepareAnswers('B');
  candidateProcess.waitForGrading('B');

  candidateProcess.cleanup();

  return 0;
}
