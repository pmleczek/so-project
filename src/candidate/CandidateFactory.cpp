#include "candidate/CandidateFactory.h"

#include "config.h"
#include "utils/random.h"

SharedCandidateState CandidateFactory::create(pid_t pid, int id, bool passed,
                                              bool reattempt) {
  double scoreA = reattempt ? Random::randomDouble(30, 100) : 0;

  SharedCandidateState candidateState = {.pid = pid,
                                         .id = id,
                                         .scoreA = scoreA,
                                         .scoreB = 0,
                                         .finalScore = 0,
                                         .passedExam = passed,
                                         .reattempt = reattempt};
  return candidateState;
}