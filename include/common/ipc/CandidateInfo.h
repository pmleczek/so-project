#pragma once

/**
 * Candidate status.
 */
enum CandidateStatus {
  Pending = 0,
  NotEligible = 1,
  PendingCommissionA = 2,
  Failed = 3,
  PendingCommissionB = 4,
  Passed = 5,
  Terminated = 6,
};

/**
 * Candidate information.
 */
struct CandidateInfo {
  int index = -1;
  int pid;
  double theoreticalScore = -1.0; // -1.0 if not graded
  double practicalScore = -1.0;   // -1.0 if not graded
  double finalScore = -1.0;       // -1.0 if not graded
  CandidateStatus status = Pending;
};
