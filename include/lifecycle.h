#pragma once

#include "ipc/SharedMemoryManager.h"

// SECTION: Main Process
void initializeMainProcess(int argc, char *argv[]);

void registerMainProcess();

void configureMainProcess(int argc, char *argv[]);

void spawnCandidateProcesses();

void waitForExamStart();
// END SECTION: Main Process

// SECTION: Candidate Process
SharedCandidateState initializeCandidate();

void runCandidateProcess();
// END SECTION: Candidate Process