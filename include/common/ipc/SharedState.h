#pragma once

enum CandidateStatus {
  Pending = 0,
  NotEligible = 1,
  PendingCommissionA = 2,
  Failed = 3,
  PendingCommissionB = 4,
  Passed = 5,
};

struct CandidateInfo {
  int pid;
  double theoreticalScore = 0.0;
  double practicalScore = 0.0;
  double finalScore = 0.0;
  CandidateStatus status = Pending;
};

// struct CommissionSeat {
//   int pid = -1;

// }

// struct CommissionInfo {

// }

struct SharedState {
  int candidateCount;
  // CommissionInfo commissionA;
  // CommissionInfo commissionB;
  CandidateInfo candidates[];
};
