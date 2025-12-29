#include <iostream>

#include "commission/CommissionProcess.h"
#include "common/ipc/SemaphoreManager.h"
#include "common/ipc/SharedMemoryManager.h"
#include "common/output/Logger.h"
#include "common/utils/Random.h"
#include <pthread.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  CommissionProcess commissionProcess(argc, argv);

  commissionProcess.waitForExamStart();
  commissionProcess.start();
  commissionProcess.cleanup();

  return 0;
}
