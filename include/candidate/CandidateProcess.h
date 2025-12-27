#pragma once

#include "candidate/CandidateConfig.h"

class CandidateProcess {
public:
  CandidateProcess()
      : config_(CandidateConfig(0, 0)) {};
  ~CandidateProcess() = default;

  void run(int argc, char *argv[]);
  const CandidateConfig &getConfig();
  void setConfig(const CandidateConfig &config);

private:
  void initialize(int argc, char *argv[]);
  void waitForEligibility();
  // TODO: Add remaining
  void cleanup();

  CandidateConfig config_;
};