#pragma once

#include "utils/random.h"

class CandidateConfig {
public:
  CandidateConfig(int id, int isReattempt)
      : candidateID(id), reattempt(isReattempt == 1) {
    if (isReattempt == 1) {
      reattemptScore = Random::randomDouble(30, 100);
    } else {
      reattemptScore = 0;
    }
  };

  int candidateID;
  bool reattempt;
  int reattemptScore;
};