#pragma once

#include "CandidateInfo.h"
#include "CommissionInfo.h"
#include <pthread.h>
#include <unistd.h>

/**
 * Shared memory structure.
 */
struct SharedState {
  /* Exam state */
  bool examStarted = false;
  int candidateCount;
  int commissionACandidateCount;
  int commissionBCandidateCount;

  /* Commission A data */
  CommissionInfo commissionA;

  /* Commission B data */
  CommissionInfo commissionB;

  /* Mutexes */
  /* Candidate data */
  pthread_mutex_t candidateMutex;
  /* Commission A data */
  pthread_mutex_t commissionAMutex;
  /* Commission B data */
  pthread_mutex_t commissionBMutex;
  /* Exam state */
  pthread_mutex_t examStateMutex;

  /* Commission PIDs */
  pid_t commissionAPID = -1;
  pid_t commissionBID = -1;

  /* Candidate data */
  CandidateInfo candidates[];
};
