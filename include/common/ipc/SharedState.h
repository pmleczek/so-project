#pragma once

#include <pthread.h>
#include <unistd.h>

enum CandidateStatus {
  Pending = 0,
  NotEligible = 1,
  PendingCommissionA = 2,
  Failed = 3,
  PendingCommissionB = 4,
  Passed = 5,
  Terminated = 6,
};

struct CandidateInfo {
  int index = -1;
  int pid;
  double theoreticalScore = -1.0;
  double practicalScore = -1.0;
  double finalScore = -1.0;
  CandidateStatus status = Pending;
};

struct CommissionSeat {
  int pid = -1;
  int questionsCount = 0;
  bool answered = false;
};

struct CommissionInfo {
  CommissionSeat seats[3];
};

struct SharedState {
  bool examStarted = false;
  int candidateCount;
  int commissionACandidateCount;
  int commissionBCandidateCount;
  CommissionInfo commissionA;
  CommissionInfo commissionB;
  pthread_mutex_t seatsMutex;
  pid_t commissionAPID = -1;
  pid_t commissionBID = -1;
  CandidateInfo candidates[];
};
