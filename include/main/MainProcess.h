#pragma once

class MainProcess {
public:
  static void run(int argc, char *argv[]);

private:
  static void initialize(int argc, char *argv[]);
  static void setupConfig(int argc, char *argv[]);
  static void waitForExamStart();
  static void cleanup();
};
