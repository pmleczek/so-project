#include "lifecycle.h"
#include "processinfo.h"

int main(int argc, char *argv[]) {
  if (!ProcessInfo::isMainProcessRegistered()) {
    initializeMainProcess(argc, argv);
  }

  return 0;
}
