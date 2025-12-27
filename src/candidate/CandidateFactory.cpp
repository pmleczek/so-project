#include "candidate/CandidateFactory.h"

#include "config.h"
#include "utils/random.h"

SharedCandidateState CandidateFactory::create(pid_t pid, bool passedExam) {
  return {.pid = pid,
          .scoreA = 0,
          .scoreB = 0,
          .finalScore = -1,
          .status = PENDING,
          .passedExam = passedExam,
          .reattempt = false};
}