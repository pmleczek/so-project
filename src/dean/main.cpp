#include "dean/DeanProcess.h"

int main(int argc, char *argv[]) {
  DeanProcess deanProcess(argc, argv);

  deanProcess.spawnComissions();
  deanProcess.spawnCandidates();

  deanProcess.waitForExamStart();
  deanProcess.verifyCandidates();
  deanProcess.start();

  deanProcess.cleanup();

  return 0;
}
