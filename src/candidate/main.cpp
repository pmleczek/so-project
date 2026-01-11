#include "candidate/CandidateProcess.h"
#include "common/ipc/SemaphoreManager.h"
#include "common/ipc/SharedMemoryManager.h"
#include "common/output/Logger.h"
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
  } else {
    Logger::info("Candidate is retaking the exam, skipping commission A");
  }

  candidateProcess.getCommissionSeat('B');
  candidateProcess.waitForQuestions('B');
  candidateProcess.prepareAnswers('B');
  candidateProcess.waitForGrading('B');

  candidateProcess.cleanup();

  return 0;
}
