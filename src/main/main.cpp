#include "main/MainProcess.h"
#include "output/Logger.h"
#include <sys/wait.h>

int main(int argc, char *argv[]) {
  MainProcess::run(argc, argv);

  int status;
  if (wait(&status) == -1) {
    Logger::shared().log("ERROR: wait failed");
    exit(1);
  }

  return 0;
}
