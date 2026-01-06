#pragma once

/**
 * Commission seat.
 */
struct CommissionSeat {
  int pid = -1; // -1 if seat is empty
  int questionsCount = 0;
  bool answered = false;
};

/**
 * Commission information.
 */
struct CommissionInfo {
  CommissionSeat seats[3];
};