#include "lifecycle.h"
#include "processinfo.h"
#include "output/Logger.h"

int main(int argc, char *argv[]) {
  if (!ProcessInfo::isMainProcessRegistered()) {
    Logger::setupLogFile();
    initializeMainProcess(argc, argv);
  }

  return 0;
}
