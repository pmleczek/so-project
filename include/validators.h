#pragma once

class ArgValidator {
public:
  static void validateArgs(int argc, char *argv[]);

private:
  static bool validateArgCount(int argc);
  static bool validateCandidateCount(int candidateCount);
  static bool validateStartTime(int startTime);
};
