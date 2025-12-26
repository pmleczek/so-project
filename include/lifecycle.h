#pragma once

// SECTION: Main Process
void initializeMainProcess(int argc, char *argv[]);

void registerMainProcess();

void configureMainProcess(int argc, char *argv[]);

void spawnCandidateProcesses();

void waitForExamStart();
// END SECTION: Main Process